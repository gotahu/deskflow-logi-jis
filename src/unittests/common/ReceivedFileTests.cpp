// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/ReceivedFile.h"
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class ReceivedFileTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void binaryUnicodeAndRepeatedTransfers()
  {
    QTemporaryDir dir;
    deskflow::ReceivedFile receiver(dir.path());
    const QByteArray data("a\0b\xff", 4);
    const auto hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    for (int n = 0; n < 2; ++n) {
      QVERIFY(receiver.begin(QStringLiteral("教材.pdf"), data.size()));
      QVERIFY(receiver.append(data.left(2)));
      QVERIFY(receiver.append(data.mid(2)));
      auto path = receiver.finish(hash);
      QCOMPARE(QFileInfo(path).fileName(), n == 0 ? QStringLiteral("教材.pdf") : QStringLiteral("教材 (1).pdf"));
      QFile file(path);
      QVERIFY(file.open(QIODevice::ReadOnly));
      QCOMPARE(file.readAll(), data);
    }
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden).size(), 2);
  }

  void interruptedAndCorruptFilesAreRemoved()
  {
    QTemporaryDir dir;
    {
      deskflow::ReceivedFile receiver(dir.path());
      QVERIFY(receiver.begin("incomplete.txt", 5));
      QVERIFY(receiver.append("ab"));
    }
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden).size(), 0);
    deskflow::ReceivedFile receiver(dir.path());
    QVERIFY(receiver.begin("wrong.txt", 1));
    QVERIFY(receiver.append("x"));
    QVERIFY(receiver.finish(QByteArray(32, '0')).isEmpty());
    QVERIFY(receiver.begin("too-large.txt", 1));
    QVERIFY(!receiver.append("xx"));
    QVERIFY(receiver.begin("short.txt", 2));
    QVERIFY(receiver.append("x"));
    QVERIFY(receiver.finish(QCryptographicHash::hash("x", QCryptographicHash::Sha256)).isEmpty());
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files | QDir::Hidden).size(), 0);
  }

  void refusesPathsAndReservedNames()
  {
    for (const auto &name : {"../escape", "C:\\escape", "/absolute", "CON.txt", "CON .txt", "LPT1", "x:y", "..", "a.", "a ", ""}) {
      QVERIFY(!deskflow::ReceivedFile::validName(name));
    }
    QVERIFY(!deskflow::ReceivedFile::validName(QString("x") + QChar(0) + "y"));
    QVERIFY(deskflow::ReceivedFile::validName("ordinary file.txt"));
  }

  void emptyFileAndExistingSymlink()
  {
    QTemporaryDir dir;
    QFile existing(dir.filePath("original"));
    QVERIFY(existing.open(QIODevice::WriteOnly));
    existing.write("keep");
    existing.close();
#ifdef Q_OS_WIN
    QVERIFY(QFile::copy(existing.fileName(), dir.filePath("file.txt")));
#else
    QVERIFY(QFile::link(existing.fileName(), dir.filePath("file.txt")));
#endif
    deskflow::ReceivedFile receiver(dir.path());
    QVERIFY(receiver.begin("file.txt", 0));
    const auto path = receiver.finish(QCryptographicHash::hash(QByteArray(), QCryptographicHash::Sha256));
    QCOMPARE(QFileInfo(path).fileName(), QStringLiteral("file (1).txt"));
    QVERIFY(existing.open(QIODevice::ReadOnly));
    QCOMPARE(existing.readAll(), QByteArray("keep"));
  }
};
QTEST_APPLESS_MAIN(ReceivedFileTests)
#include "ReceivedFileTests.moc"
