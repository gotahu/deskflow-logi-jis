// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "gui/FileTransferWindow.h"
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
