// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "net/FileTransferSession.h"
#include <QDir>
#include <QJsonDocument>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QtEndian>
using deskflow::FileTransferSession;
namespace {
QByteArray packet(char type, QByteArray body) {
  QByteArray bytes(4, Qt::Uninitialized);
  qToBigEndian<quint32>(body.size()+1, bytes.data());
  return bytes + type + body;
}
QByteArray json(QJsonObject o) { return QJsonDocument(o).toJson(QJsonDocument::Compact); }
void writeFile(const QString &path, const QByteArray &bytes) { QFile f(path); QVERIFY(f.open(QIODevice::WriteOnly)); QCOMPARE(f.write(bytes),bytes.size()); }
QByteArray readFile(const QString &path) { QFile f(path); if (!f.open(QIODevice::ReadOnly)) return {}; return f.readAll(); }
}
class FileTransferSessionTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void bidirectionalLargeUnicodeAndCollision()
  {
    QTemporaryDir mac, win, source;
    const QByteArray data(3*1024*1024+3, '\xff');
    const auto macSource = source.filePath(QStringLiteral("授業資料.pdf"));
    const auto winSource = source.filePath("empty.txt");
    writeFile(macSource, data); writeFile(winSource, {});
    writeFile(win.filePath(QStringLiteral("授業資料.pdf")), "keep");
    QTcpServer listener; QVERIFY(listener.listen(QHostAddress::LocalHost));
    QTcpSocket left; left.connectToHost(QHostAddress::LocalHost, listener.serverPort());
    QTRY_VERIFY(listener.hasPendingConnections());
    std::unique_ptr<QTcpSocket> right(listener.nextPendingConnection());
    FileTransferSession a(&left,mac.path(),"Mac"), b(right.get(),win.path(),"Windows");
    QSignalSpy aReceived(&a,&FileTransferSession::received), bReceived(&b,&FileTransferSession::received);
    QSignalSpy aSent(&a,&FileTransferSession::sent), bSent(&b,&FileTransferSession::sent);
    QSignalSpy controls(&b,&FileTransferSession::control), failuresA(&a,&FileTransferSession::failed), failuresB(&b,&FileTransferSession::failed);
    QTRY_COMPARE(a.peerName(),QString("Windows")); QTRY_COMPARE(b.peerName(),QString("Mac"));
    a.sendFiles({macSource}); b.sendFiles({winSource}); a.sendControl({{"event","screen"},{"target","Windows"}});
    QTRY_COMPARE_WITH_TIMEOUT(bReceived.size(),1,10000);
    QTRY_COMPARE(aReceived.size(),1); QTRY_COMPARE(aSent.size(),1); QTRY_COMPARE(bSent.size(),1);
    QCOMPARE(readFile(bReceived.first().first().toString()),data);
    QCOMPARE(readFile(win.filePath(QStringLiteral("授業資料.pdf"))),QByteArray("keep"));
    QCOMPARE(QFileInfo(bReceived.first().first().toString()).fileName(),QStringLiteral("授業資料 (1).pdf"));
    QCOMPARE(QFileInfo(aReceived.first().first().toString()).size(),qint64(0));
    QCOMPARE(controls.size(),1); QCOMPARE(failuresA.size(),0); QCOMPARE(failuresB.size(),0);
    QCOMPARE(QDir(mac.path()).entryList(QDir::Files|QDir::Hidden).size(),1);
    QCOMPARE(QDir(win.path()).entryList(QDir::Files|QDir::Hidden).size(),2);
  }
  void malformedAndDisconnectedTransfersLeaveNoFile()
  {
    for (const bool oversize : {true,false}) {
      QTemporaryDir dir;
      QTcpServer listener; QVERIFY(listener.listen(QHostAddress::LocalHost));
      QTcpSocket sender; sender.connectToHost(QHostAddress::LocalHost,listener.serverPort());
      QTRY_VERIFY(listener.hasPendingConnections());
      std::unique_ptr<QTcpSocket> receiver(listener.nextPendingConnection());
      FileTransferSession session(receiver.get(),dir.path(),"Receiver");
      QSignalSpy failed(&session,&FileTransferSession::failed);
      sender.write(packet('H',json({{"version",1},{"name","Sender"}})));
      QTRY_COMPARE(session.peerName(),QString("Sender"));
      if (oversize) {
        QByteArray bytes(4, Qt::Uninitialized); qToBigEndian<quint32>(0x7fffffff,bytes.data()); sender.write(bytes);
        QTRY_VERIFY(!failed.isEmpty());
      } else {
        sender.write(packet('B',json({{"id","12345678-1234-1234-1234-123456789012"},{"name","unfinished.pdf"},{"size","100000"}})));
        sender.write(packet('D',QByteArray(1024,'x'))); sender.flush();
        QTRY_VERIFY(!QDir(dir.path()).entryList(QDir::Files|QDir::Hidden).isEmpty());
        sender.abort(); QTRY_VERIFY(!failed.isEmpty());
      }
      QCOMPARE(QDir(dir.path()).entryList(QDir::Files|QDir::Hidden).size(),0);
    }
  }
};
QTEST_GUILESS_MAIN(FileTransferSessionTests)
#include "FileTransferSessionTests.moc"
