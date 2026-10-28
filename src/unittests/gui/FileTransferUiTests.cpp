// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "gui/FileTransferWindow.h"
#include "gui/FileDropArea.h"
#include "gui/FileDragGesture.h"
#include <QDir>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QUrl>
#include "common/Settings.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
class FileTransferUiTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void clicksDoNotActivateEdgeTargets()
  {
    deskflow::gui::FileDragGesture gesture;
    const QPoint origin(100,100);
    QVERIFY(!gesture.update(true,false,origin,10));
    for (int sample=0; sample<20; ++sample) QVERIFY(!gesture.update(true,true,origin,10));
    QVERIFY(!gesture.update(true,true,origin+QPoint(2,2),10));
    QVERIFY(gesture.update(true,true,origin+QPoint(10,0),10));
    QVERIFY(gesture.update(true,true,origin,10));
    QVERIFY(!gesture.update(true,false,origin,10));
    QVERIFY(!gesture.update(true,true,origin,10));
    QVERIFY(gesture.update(true,true,origin+QPoint(20,0),10));
    QVERIFY(!gesture.update(false,true,origin,10));
    QVERIFY(!gesture.update(true,true,origin,10));
  }
  void edgeTargetsDoNotPaintFramesOrHoverHighlights()
  {
    deskflow::gui::FileDropArea edge(nullptr,true);
    edge.resize(80,16);
    QVERIFY(edge.testAttribute(Qt::WA_TranslucentBackground));
    QVERIFY(edge.windowFlags().testFlag(Qt::NoDropShadowWindowHint));
    auto checkAlpha = [&] {
      const auto pixels = edge.grab().toImage();
      QVERIFY(pixels.hasAlphaChannel());
      for (int y=0; y<pixels.height(); ++y)
        for (int x=0; x<pixels.width(); ++x)
          QCOMPARE(pixels.pixelColor(x,y).alpha(),1);
    };
    checkAlpha();
    QStringList armed;
    edge.armed = [&](const QStringList &paths) { armed=paths; };
    QMimeData files;
    files.setUrls({QUrl::fromLocalFile(QDir::temp().filePath("edge-test.txt"))});
    QDragEnterEvent enter(QPoint(4,4),Qt::CopyAction,&files,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(&edge,&enter);
    QVERIFY(enter.isAccepted());
    QCOMPARE(armed.size(),1);
    checkAlpha();
    deskflow::gui::FileDropArea normal(nullptr,false);
    normal.resize(100,120);
    QVERIFY(normal.grab().toImage().pixelColor(50,50).alpha()>1);
  }
  void stoppedWindowFitsAndCanDisableDrag()
  {
    QTemporaryDir dir;
    qputenv("XDG_CONFIG_HOME",dir.path().toUtf8());
    qputenv("XDG_STATE_HOME",dir.path().toUtf8());
    Settings::setSettingsFile(dir.filePath("settings.conf"));
    Settings::setValue(Settings::Core::ComputerName,"Test Mac");
    ServerConfig config;
    deskflow::gui::CoreProcess core(config);
    deskflow::gui::FileTransferWindow window(&core);
    window.show();
    QTest::qWait(40);
    QVERIFY(window.height() <= 720);
    QVERIFY(!window.findChildren<QComboBox *>().isEmpty());
    bool sawOpen = false, sawChoose = false;
    for (auto *button : window.findChildren<QPushButton *>()) {
      sawOpen |= button->text().contains(QStringLiteral("ダウンロード"));
      sawChoose |= button->text().contains(QStringLiteral("ファイルを選択"));
    }
    QVERIFY(sawOpen && sawChoose);
    for (auto *box : window.findChildren<QCheckBox *>()) {
      if (box->text().contains(QStringLiteral("画面をまたぐ"))) {
        QVERIFY(box->isChecked()); box->setChecked(false);
        QVERIFY(!Settings::value(Settings::Files::CrossScreen).toBool());
        Settings::removeUnknownComputers({});
        QVERIFY(!Settings::value(Settings::Files::CrossScreen).toBool());
      }
    }
    const auto output = qEnvironmentVariable("DESKFLOW_TRANSFER_SCREENSHOT");
    if (!output.isEmpty()) QVERIFY(window.grab().save(output));
  }
};
QTEST_MAIN(FileTransferUiTests)
#include "FileTransferUiTests.moc"
