// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once

#include <QCryptographicHash>
#include <QTemporaryFile>
#include <QString>
#include <memory>

namespace deskflow {

// Stores a streamed file in a fixed destination. No sender-supplied path is
// ever used; incomplete transfers remain temporary and existing files survive.
class ReceivedFile
{
public:
  explicit ReceivedFile(QString destination);
  bool begin(const QString &name, qint64 size);
  bool append(const QByteArray &data);
  QString finish(const QByteArray &sha256);
  void abort();
  QString error() const { return m_error; }
  static bool validName(const QString &name);

private:
  bool fail(const QString &message);
  QString m_destination, m_name, m_error;
  qint64 m_expected = 0, m_received = 0;
  std::unique_ptr<QTemporaryFile> m_file;
  QCryptographicHash m_hash{QCryptographicHash::Sha256};
};
} // namespace deskflow
