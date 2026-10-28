// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "net/FileTransferSession.h"
#include <QPointer>
#include <QSet>
#include <QSslConfiguration>
#include <QTcpServer>
#include <QTimer>
#include <functional>
class QSslSocket;
namespace deskflow::gui {
class FileTransferHub : public QObject
{
  Q_OBJECT
public:
  explicit FileTransferHub(QString destination, QObject *parent = nullptr);
  void configure(bool enabled, bool server, const QString &host, quint16 port,
                 const QString &name, const QString &pem, const QString &trustedDb, const QString &bindAddress = {});
  void sendFiles(const QString &peer, const QStringList &paths);
  void broadcast(const QJsonObject &control);
  void cancel();
  QStringList peers() const;
  bool busy() const;
  bool serverMode() const { return m_serverMode; }
  QString localName() const { return m_name; }
Q_SIGNALS:
  void peersChanged();
  void activityChanged();
  void controlReceived(const QString &peer, const QJsonObject &control);
  void status(const QString &message);
  void progress(qint64 done, qint64 total);
  void received(const QString &path);
  void sent(const QString &name);
  void failed(const QString &message);
private:
  class Listener : public QTcpServer {
  public:
    using QTcpServer::QTcpServer;
    std::function<void(qintptr)> accept;
    void incomingConnection(qintptr descriptor) override { accept(descriptor); }
  };
  void attach(QSslSocket *socket, bool incoming);
  void attempt();
  bool trusted(QSslSocket *socket) const;
  QString m_destination, m_name, m_host, m_trustedDb, m_configKey;
  quint16 m_port = 0;
  bool m_enabled = false, m_serverMode = false;
  QSslConfiguration m_tls;
  Listener m_listener;
  QSet<QSslSocket *> m_sockets;
  QSet<deskflow::FileTransferSession *> m_sessions;
  QTimer m_retry;
};
}
