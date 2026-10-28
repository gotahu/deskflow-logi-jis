// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "common/ReceivedFile.h"
#include <QAbstractSocket>
#include <QFile>
#include <QJsonObject>
#include <QObject>
#include <QQueue>
#include <QTimer>

namespace deskflow {
// Construct only after the transport has authenticated the peer.
class FileTransferSession : public QObject
{
  Q_OBJECT
public:
  FileTransferSession(QAbstractSocket *socket, QString destination, QString localName, QObject *parent = nullptr);
  void sendFiles(const QStringList &paths);
  void sendControl(const QJsonObject &control);
  QString peerName() const { return m_peer; }
  bool busy() const { return !m_failed && (!m_sendId.isEmpty() || !m_receiveId.isEmpty()); }
Q_SIGNALS:
  void ready();
  void activityChanged();
  void control(const QJsonObject &control);
  void status(const QString &message);
  void progress(qint64 done, qint64 total);
  void received(const QString &path);
  void sent(const QString &name);
  void failed(const QString &message);
private:
  void write(char type, const QByteArray &data);
  void read();
  bool frame(char type, const QByteArray &data);
  void pump();
  void fail(const QString &message);
  QAbstractSocket *m_socket;
  QString m_peer, m_local, m_sendId, m_receiveId, m_sendName;
  QByteArray m_buffer;
  QQueue<QString> m_queue;
  QFile m_source;
  qint64 m_remaining = 0, m_total = 0, m_inTotal = 0, m_inDone = 0;
  bool m_waitingAck = false, m_failed = false;
  ReceivedFile m_received;
  QCryptographicHash m_hash{QCryptographicHash::Sha256};
  QTimer m_timeout;
};
}
