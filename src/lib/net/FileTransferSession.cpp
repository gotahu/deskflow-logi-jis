// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "FileTransferSession.h"
#include <QFileInfo>
#include <QJsonDocument>
#include <QUuid>
#include <QtEndian>

namespace deskflow {
namespace {
QByteArray json(const QJsonObject &o) { return QJsonDocument(o).toJson(QJsonDocument::Compact); }
constexpr quint32 maxFrame = 65537;
}
FileTransferSession::FileTransferSession(QAbstractSocket *socket, QString destination, QString localName, QObject *parent)
    : QObject(parent), m_socket(socket), m_local(std::move(localName)), m_received(std::move(destination))
{
  connect(socket, &QIODevice::readyRead, this, &FileTransferSession::read);
  connect(socket, &QIODevice::bytesWritten, this, [this] { pump(); });
  connect(socket, &QAbstractSocket::disconnected, this, [this] {
    m_received.abort();
    if (!m_receiveId.isEmpty() || !m_sendId.isEmpty() || !m_queue.isEmpty())
      fail(tr("接続が切れたため転送を中止しました。再接続後に送り直してください。"));
  });
  m_timeout.setInterval(60000);
  m_timeout.setSingleShot(true);
  connect(&m_timeout, &QTimer::timeout, this, [this] { fail(tr("ファイル転送がタイムアウトしました。")); });
  m_timeout.start();
  write('H', json({{"version", 1}, {"name", m_local}}));
  QTimer::singleShot(0, this, &FileTransferSession::read);
}
void FileTransferSession::write(char type, const QByteArray &data)
{
  if (m_failed) return;
  const quint32 size = static_cast<quint32>(data.size() + 1);
  QByteArray packet(4, Qt::Uninitialized);
  qToBigEndian(size, packet.data());
  packet += type;
  packet += data;
  if (m_socket->write(packet) != packet.size()) fail(tr("転送データを書き込めませんでした。"));
}
void FileTransferSession::sendControl(const QJsonObject &o) { write('C', json(o)); }
void FileTransferSession::sendFiles(const QStringList &paths)
{
  if (m_failed) return;
  if (m_queue.size() + paths.size() > 1024) { Q_EMIT failed(tr("一度に送るファイルは1024個以内にしてください。")); return; }
  for (const auto &path : paths) {
    const QFileInfo info(path);
    if (!info.isFile() || !ReceivedFile::validName(info.fileName().normalized(QString::NormalizationForm_C))) {
      Q_EMIT failed(tr("送信できないファイルです: %1。フォルダーは ZIP にしてください。").arg(info.fileName()));
      continue;
    }
    m_queue.enqueue(info.absoluteFilePath());
  }
  pump();
}
void FileTransferSession::pump()
{
  if (m_failed || m_peer.isEmpty() || m_waitingAck || m_socket->state() != QAbstractSocket::ConnectedState) return;
  // Keep less than 256 KiB queued, so pointer-control messages remain responsive.
  if (m_socket->bytesToWrite() > 262144) return;
  if (!m_source.isOpen()) {
    if (m_queue.isEmpty()) return;
    const auto path = m_queue.dequeue();
    m_source.setFileName(path);
    if (!m_source.open(QIODevice::ReadOnly)) { fail(tr("ファイルを開けません: %1").arg(path)); return; }
    m_sendId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_sendName = QFileInfo(path).fileName().normalized(QString::NormalizationForm_C);
    m_total = m_remaining = m_source.size();
    m_hash.reset();
    Q_EMIT activityChanged();
    write('B', json({{"id", m_sendId}, {"name", m_sendName}, {"size", QString::number(m_total)}}));
    Q_EMIT status(tr("送信中: %1").arg(m_sendName));
  }
  while (m_remaining > 0 && m_socket->bytesToWrite() <= 262144 && !m_failed) {
    const auto data = m_source.read(qMin<qint64>(65536, m_remaining));
    if (data.isEmpty()) { fail(tr("送信元のファイルが変更されたか、読み取れません。")); return; }
    m_hash.addData(data);
    m_remaining -= data.size();
    write('D', data);
  }
  Q_EMIT progress(m_total - m_remaining, m_total);
  m_timeout.start();
  if (m_remaining == 0 && !m_failed) {
    m_source.close();
    m_waitingAck = true;
    write('E', m_hash.result());
  }
}
void FileTransferSession::read()
{
  if (m_failed) return;
  // A peer may coalesce frames; never buffer an unbounded packet.
  while (m_socket->bytesAvailable() > 0) {
    m_buffer += m_socket->read(qMin<qint64>(65541 - m_buffer.size(), m_socket->bytesAvailable()));
    while (m_buffer.size() >= 4) {
      const auto length = qFromBigEndian<quint32>(m_buffer.constData());
      if (length < 1 || length > maxFrame) { fail(tr("不正なファイル転送パケットです。")); return; }
      if (m_buffer.size() < length + 4) break;
      const auto type = m_buffer.at(4);
      const auto data = m_buffer.mid(5, length - 1);
      m_buffer.remove(0, length + 4);
      if (!frame(type, data)) { fail(tr("ファイル転送の内容または順序が不正です。")); return; }
      if (m_failed) return;
      if (m_sendId.isEmpty() && m_receiveId.isEmpty() && !m_peer.isEmpty()) m_timeout.stop();
      else m_timeout.start();
    }
  }
}
bool FileTransferSession::frame(char type, const QByteArray &data)
{
  if (type == 'H') {
    const auto o = QJsonDocument::fromJson(data).object();
    if (!m_peer.isEmpty() || o.value("version").toInt() != 1) return false;
    m_peer = o.value("name").toString();
    if (m_peer.isEmpty() || m_peer.size() > 256 || m_peer.contains('\n')) return false;
    Q_EMIT ready();
    pump();
    return true;
  }
  if (m_peer.isEmpty()) return false;
  if (type == 'C') {
    if (data.size() > 8192) return false;
    const auto doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return false;
    Q_EMIT control(doc.object());
    return true;
  }
  if (type == 'B') {
    if (!m_receiveId.isEmpty() || data.size() > 8192) return false;
    const auto o = QJsonDocument::fromJson(data).object();
    const auto id = o.value("id").toString();
    bool ok = false;
    const auto size = o.value("size").toString().toLongLong(&ok);
    if (id.size() != 36 || !ok || !m_received.begin(o.value("name").toString(), size)) return false;
    m_receiveId = id;
    m_inTotal = size; m_inDone = 0;
    Q_EMIT activityChanged();
    Q_EMIT status(tr("受信中: %1").arg(o.value("name").toString()));
    return true;
  }
  if (type == 'D') {
    if (m_receiveId.isEmpty() || !m_received.append(data)) return false;
    m_inDone += data.size();
    Q_EMIT progress(m_inDone, m_inTotal);
    return true;
  }
  if (type == 'E') {
    if (m_receiveId.isEmpty()) return false;
    const auto path = m_received.finish(data);
    if (path.isEmpty()) { fail(m_received.error()); return true; }
    write('A', json({{"id", m_receiveId}, {"name", QFileInfo(path).fileName()}}));
    m_receiveId.clear();
    Q_EMIT activityChanged();
    Q_EMIT received(path);
    return true;
  }
  if (type == 'A') {
    const auto o = QJsonDocument::fromJson(data).object();
    if (!m_waitingAck || o.value("id").toString() != m_sendId) return false;
    m_waitingAck = false;
    m_sendId.clear();
    Q_EMIT activityChanged();
    Q_EMIT sent(o.value("name").toString());
    pump();
    return true;
  }
  return false;
}
void FileTransferSession::fail(const QString &message)
{
  if (m_failed) return;
  m_failed = true;
  m_timeout.stop();
  m_source.close();
  m_received.abort();
  m_queue.clear();
  Q_EMIT activityChanged();
  m_socket->abort();
  Q_EMIT failed(message);
}
}
