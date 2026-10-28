// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "FileTransferHub.h"
#include "net/FingerprintDatabase.h"
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QSslCertificate>
#include <QSslKey>
#include <QSslSocket>

namespace deskflow::gui {
FileTransferHub::FileTransferHub(QString destination, QObject *parent)
    : QObject(parent), m_destination(std::move(destination)), m_listener(this)
{
  m_listener.accept = [this](qintptr descriptor) {
    auto *socket = new QSslSocket(this);
    if (!socket->setSocketDescriptor(descriptor)) { socket->deleteLater(); return; }
    if (m_sockets.size() >= 8) { socket->abort(); socket->deleteLater(); return; }
    attach(socket, true);
  };
  m_retry.setInterval(5000);
  connect(&m_retry, &QTimer::timeout, this, &FileTransferHub::attempt);
}
void FileTransferHub::configure(bool enabled, bool server, const QString &host, quint16 port,
                                const QString &name, const QString &pem, const QString &trustedDb,
                                const QString &bindAddress)
{
  const auto key = QStringList{QString::number(enabled), QString::number(server), host, QString::number(port),
                              name, pem, trustedDb, bindAddress, QString::number(QFileInfo(pem).lastModified().toMSecsSinceEpoch())}.join('\n');
  if (key == m_configKey) return;
  m_configKey = key;
  m_enabled = false;
  cancel();
  m_listener.close();
  m_retry.stop();
  if (!enabled) { Q_EMIT status(tr("ファイル転送は停止中です。")); return; }
  QFile file(pem);
  if (!QSslSocket::supportsSsl() || !file.open(QIODevice::ReadOnly)) {
    Q_EMIT failed(tr("暗号化用の証明書を読み込めません。Deskflow の TLS 設定を確認してください。")); return;
  }
  const auto contents = file.readAll();
  const auto certs = QSslCertificate::fromData(contents);
  const QSslKey privateKey(contents, QSsl::Rsa);
  if (certs.isEmpty() || privateKey.isNull()) { Q_EMIT failed(tr("TLS 証明書が不正です。")); return; }
  m_tls = QSslConfiguration::defaultConfiguration();
  m_tls.setProtocol(QSsl::TlsV1_2OrLater);
  m_tls.setLocalCertificateChain(certs);
  m_tls.setPrivateKey(privateKey);
  m_tls.setPeerVerifyMode(QSslSocket::VerifyPeer);
  m_name = name;
  m_host = host;
  m_port = port;
  m_trustedDb = trustedDb;
  m_serverMode = server;
  m_enabled = true;
  if (server) {
    const QHostAddress address = bindAddress.isEmpty() ? QHostAddress(QHostAddress::Any) : QHostAddress(bindAddress);
    if (address.isNull() || !m_listener.listen(address, port)) {
      Q_EMIT failed(tr("ファイル転送の待受けを開始できません: %1").arg(m_listener.errorString()));
      m_enabled = false;
      return;
    }
    Q_EMIT status(tr("接続待ち。両方の PC でこの改良版を起動してください。"));
  } else {
    m_retry.start();
    attempt();
  }
}
void FileTransferHub::cancel()
{
  const auto sockets = m_sockets;
  for (auto *socket : sockets) {
    socket->abort();
    m_sockets.remove(socket);
    socket->deleteLater();
  }
}
bool FileTransferHub::trusted(QSslSocket *socket) const
{
  const auto cert = socket->peerCertificate();
  if (cert.isNull() || cert.isBlacklisted() || cert.expiryDate() < QDateTime::currentDateTimeUtc() ||
      cert.effectiveDate() > QDateTime::currentDateTimeUtc()) return false;
  FingerprintDatabase db;
  db.read(m_trustedDb);
  return db.isTrusted({QCryptographicHash::Sha256, cert.digest(QCryptographicHash::Sha256)});
}
void FileTransferHub::attach(QSslSocket *socket, bool incoming)
{
  m_sockets.insert(socket);
  socket->setSslConfiguration(m_tls);
  socket->setReadBufferSize(262144);
  connect(socket, &QSslSocket::sslErrors, this, [this, socket](const QList<QSslError> &errors) {
    if (!trusted(socket)) return;
    for (const auto &error : errors) {
      switch (error.error()) {
      case QSslError::SelfSignedCertificate:
      case QSslError::SelfSignedCertificateInChain:
      case QSslError::CertificateUntrusted:
      case QSslError::UnableToGetLocalIssuerCertificate:
      case QSslError::UnableToVerifyFirstCertificate:
      case QSslError::HostNameMismatch: break;
      default: return;
      }
    }
    // The existing Deskflow SHA-256 pin is our trust anchor, not a public CA.
    socket->ignoreSslErrors(errors);
  });
  connect(socket, &QSslSocket::encrypted, this, [this, socket] {
    if (!trusted(socket)) { socket->abort(); Q_EMIT failed(tr("Deskflow で未承認の相手からの転送接続を拒否しました。")); return; }
    auto *session = new deskflow::FileTransferSession(socket, m_destination, m_name, socket);
    m_sessions.insert(session);
    connect(session, &QObject::destroyed, this, [this, session] {
      if (m_sessions.remove(session)) Q_EMIT peersChanged();
    });
    connect(session, &deskflow::FileTransferSession::ready, this, [this, session, socket] {
      for (auto *other : m_sessions) {
        if (other != session && other->peerName() == session->peerName()) { socket->abort(); return; }
      }
      Q_EMIT peersChanged();
      Q_EMIT status(tr("%1 とファイルを送受信できます。").arg(session->peerName()));
    });
    connect(session, &deskflow::FileTransferSession::control, this, [this, session](const QJsonObject &o) {
      Q_EMIT controlReceived(session->peerName(), o);
    });
    connect(session, &deskflow::FileTransferSession::activityChanged, this, &FileTransferHub::activityChanged);
    connect(session, &deskflow::FileTransferSession::status, this, &FileTransferHub::status);
    connect(session, &deskflow::FileTransferSession::progress, this, &FileTransferHub::progress);
    connect(session, &deskflow::FileTransferSession::received, this, &FileTransferHub::received);
    connect(session, &deskflow::FileTransferSession::sent, this, &FileTransferHub::sent);
    connect(session, &deskflow::FileTransferSession::failed, this, &FileTransferHub::failed);
    connect(socket, &QAbstractSocket::disconnected, this, [this, session] {
      m_sessions.remove(session);
      Q_EMIT peersChanged();
    });
  });
  connect(socket, &QAbstractSocket::disconnected, this, [this, socket] {
    m_sockets.remove(socket);
    socket->deleteLater();
    Q_EMIT status(tr("ファイル転送の接続が切れました。再接続を待っています。"));
  });
  connect(socket, &QAbstractSocket::errorOccurred, this, [this, socket] {
    Q_EMIT failed(tr("ファイル転送に接続できません: %1（両側の改良版と TCP %2 を確認）")
                     .arg(socket->errorString()).arg(m_port));
    m_sockets.remove(socket);
    socket->deleteLater();
  });
  QTimer::singleShot(10000, socket, [socket] { if (!socket->isEncrypted()) socket->abort(); });
  if (incoming) socket->startServerEncryption();
  else socket->connectToHostEncrypted(m_host, m_port);
}
void FileTransferHub::attempt()
{
  if (!m_enabled || m_serverMode || !m_sockets.isEmpty() || m_host.isEmpty()) return;
  attach(new QSslSocket(this), false);
}
bool FileTransferHub::busy() const
{
  for (auto *session : m_sessions) if (session->busy()) return true;
  return false;
}
QStringList FileTransferHub::peers() const
{
  QStringList result;
  for (auto *session : m_sessions) if (!session->peerName().isEmpty()) result.append(session->peerName());
  result.sort();
  return result;
}
void FileTransferHub::sendFiles(const QString &peer, const QStringList &paths)
{
  for (auto *session : m_sessions) if (session->peerName() == peer) { session->sendFiles(paths); return; }
  Q_EMIT failed(tr("送信先が接続されていません。両側の Deskflow を確認してください。"));
}
void FileTransferHub::broadcast(const QJsonObject &control)
{
  const auto sessions = m_sessions;
  for (auto *session : sessions) if (!session->peerName().isEmpty()) session->sendControl(control);
}
}
