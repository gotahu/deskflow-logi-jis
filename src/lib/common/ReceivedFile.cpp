// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "ReceivedFile.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#endif

namespace deskflow {

ReceivedFile::ReceivedFile(QString destination) : m_destination(std::move(destination)) {}

bool ReceivedFile::validName(const QString &name)
{
  if (name.isEmpty() || name == "." || name == ".." || name.toUtf8().size() > 200 ||
      name.endsWith('.') || name.endsWith(' ')) return false;
  for (const auto c : name) {
    if (c.unicode() < 32 || QStringLiteral("/\\:<>\"|?*").contains(c)) return false;
  }
  const auto stem = name.section('.', 0, 0).trimmed().toUpper();
  static const QRegularExpression reserved(QStringLiteral("^(CON|PRN|AUX|NUL|COM[1-9¹²³]|LPT[1-9¹²³])$"));
  return !reserved.match(stem).hasMatch();
}

bool ReceivedFile::begin(const QString &name, qint64 size)
{
  abort();
  m_error.clear();
  m_name = name.normalized(QString::NormalizationForm_C);
  if (!validName(m_name) || size < 0) return fail(QStringLiteral("Invalid file name or size"));
  if (!QDir::isAbsolutePath(m_destination) || !QDir(m_destination).exists()) return fail(QStringLiteral("Downloads directory is unavailable"));
  m_file = std::make_unique<QTemporaryFile>(QDir(m_destination).filePath(QStringLiteral(".deskflow-receiving-XXXXXX")));
  if (!m_file->open()) return fail(m_file->errorString());
#ifndef Q_OS_WIN
  m_file->setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
#endif
  m_expected = size;
  m_received = 0;
  m_hash.reset();
  return true;
}

bool ReceivedFile::append(const QByteArray &data)
{
  if (!m_file || !m_file->isOpen()) return fail(QStringLiteral("No active transfer"));
  if (data.size() > m_expected - m_received) return fail(QStringLiteral("File exceeds its declared size"));
  if (m_file->write(data) != data.size()) return fail(m_file->errorString());
  m_hash.addData(data);
  m_received += data.size();
  return true;
}

QString ReceivedFile::finish(const QByteArray &sha256)
{
  if (!m_file || !m_file->isOpen() || m_received != m_expected || sha256.size() != 32 || m_hash.result() != sha256) {
    fail(QStringLiteral("Incomplete file or checksum mismatch"));
    return {};
  }
  if (!m_file->flush()) {
    fail(m_file->errorString());
    return {};
  }
  const auto temporary = m_file->fileName();
  m_file->close();
#ifdef Q_OS_WIN
  // QTemporaryFile keeps its native handle after close(). Destroy it before
  // MoveFileExW; otherwise Windows can reject the move as a sharing violation.
  m_file->setAutoRemove(false);
  m_file.reset();
#endif
  const QFileInfo name(m_name);
  const auto suffix = name.completeSuffix();
  const auto base = suffix.isEmpty() ? m_name : m_name.left(m_name.size() - suffix.size() - 1);
  for (int n = 0; n < 10000; ++n) {
    const auto candidate = n == 0 ? m_name : base + QStringLiteral(" (%1)").arg(n) +
                                                   (suffix.isEmpty() ? QString() : '.' + suffix);
    const auto target = QDir(m_destination).filePath(candidate);
    // Publish atomically with no replacement, even if a file appears after
    // choosing the name. Both paths are inside the same Downloads directory.
#ifdef Q_OS_WIN
    const bool published = MoveFileExW(reinterpret_cast<LPCWSTR>(temporary.utf16()),
                                      reinterpret_cast<LPCWSTR>(target.utf16()), MOVEFILE_WRITE_THROUGH) != 0;
#else
    const bool published = ::link(QFile::encodeName(temporary).constData(), QFile::encodeName(target).constData()) == 0;
#endif
    if (published) {
#ifndef Q_OS_WIN
      m_file.reset(); // Removes the temporary link on Unix; the final file remains.
#endif
      return target;
    }
    if (!QFileInfo::exists(target) && !QFileInfo(target).isSymLink()) break;
  }
#ifdef Q_OS_WIN
  QFile::remove(temporary);
#endif
  fail(QStringLiteral("Could not save the received file without overwriting an existing file"));
  return {};
}

void ReceivedFile::abort()
{
  m_file.reset();
  m_expected = m_received = 0;
}

bool ReceivedFile::fail(const QString &message)
{
  m_error = message;
  abort();
  return false;
}
} // namespace deskflow
