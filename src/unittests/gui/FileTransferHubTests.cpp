// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "gui/FileTransferHub.h"
#include "net/SecureUtils.h"
#include "net/FingerprintDatabase.h"
#include <QSslCertificate>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
using deskflow::gui::FileTransferHub;
class FileTransferHubTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void pinnedTlsRoundTripAndUnknownPeer()
  {
    QTemporaryDir dir;
    const auto pemA=dir.filePath("server.pem"), pemB=dir.filePath("client.pem");
    deskflow::generatePemSelfSignedCert(pemA,2048); deskflow::generatePemSelfSignedCert(pemB,2048);
    const auto pinA=QSslCertificate::fromPath(pemA).first().digest(QCryptographicHash::Sha256);
    const auto pinB=QSslCertificate::fromPath(pemB).first().digest(QCryptographicHash::Sha256);
    FingerprintDatabase dbA,dbB;
    dbA.addTrusted({QCryptographicHash::Sha256,pinB}); dbB.addTrusted({QCryptographicHash::Sha256,pinA});
    const auto pathA=dir.filePath("trustA"), pathB=dir.filePath("trustB");
    QVERIFY(dbA.write(pathA)); QVERIFY(dbB.write(pathB));
    for (const bool trusted : {true,false}) {
      QTcpServer reservation; QVERIFY(reservation.listen(QHostAddress::LocalHost));
      const auto port=reservation.serverPort(); reservation.close();
      FileTransferHub a(dir.path()), b(dir.path());
      QSignalSpy errorsA(&a,&FileTransferHub::failed), errorsB(&b,&FileTransferHub::failed);
      a.configure(true,true,"",port,"Mac",pemA,trusted ? pathA : dir.filePath("unknown"),"127.0.0.1");
      b.configure(true,false,"127.0.0.1",port,"Windows",pemB,pathB);
      if (trusted) {
        QTRY_VERIFY_WITH_TIMEOUT(!a.peers().isEmpty() || !errorsA.isEmpty() || !errorsB.isEmpty(),15000);
        QVERIFY2(!a.peers().isEmpty(),qPrintable((errorsA.isEmpty()?QString():errorsA.first().first().toString())+
                                                (errorsB.isEmpty()?QString():errorsB.first().first().toString())));
        QTRY_COMPARE(b.peers(),QStringList{"Mac"});
        QFile source(dir.filePath("test-file.txt")); QVERIFY(source.open(QIODevice::WriteOnly)); source.write("TLS test"); source.close();
        QSignalSpy received(&b,&FileTransferHub::received);
        a.sendFiles("Windows",{source.fileName()});
        QTRY_COMPARE(received.size(),1);
        QFile result(received.first().first().toString()); QVERIFY(result.open(QIODevice::ReadOnly));
        QCOMPARE(result.readAll(),QByteArray("TLS test"));
      } else {
        QTRY_VERIFY_WITH_TIMEOUT(!errorsA.isEmpty() || !errorsB.isEmpty(),15000);
        QVERIFY(a.peers().isEmpty()); QVERIFY(b.peers().isEmpty());
      }
    }
  }
};
QTEST_GUILESS_MAIN(FileTransferHubTests)
#include "FileTransferHubTests.moc"
