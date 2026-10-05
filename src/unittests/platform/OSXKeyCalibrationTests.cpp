/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "platform/OSXKeyCalibration.h"
#include "base/Log.h"
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

class OSXKeyCalibrationTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void loadedMappingsPreserveShiftAndOverrides()
  {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const auto path = directory.filePath("keyboard-calibration.json");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    const QByteArray json = R"({
      "mappings": {
        "left-parenthesis": {
          "local": {"qt_key":"0x0028", "preferred_keycode":"0x001C", "modifiers":"shift"},
          "remote": {"preferred_keycode":"0x001C", "modifiers":"shift"}
        },
        "right-parenthesis": {
          "local": {"qt_key":"0x0029", "preferred_keycode":"0x0019", "modifiers":"shift"},
          "remote": {"preferred_keycode":"0x0019", "modifiers":"shift"}
        }
      },
      "overrides": [{"match":{"id":"F13"},"send":{"id":"Control_L"}}]
    })";
    QCOMPARE(file.write(json), qint64(json.size()));
    file.close();
    Log log;
    OSXKeyCalibration calibration(path.toStdString());
    QVERIFY(!calibration.empty());
    const auto *left = calibration.find(0x1D, KeyModifierShift, '(');
    QVERIFY(left != nullptr);
    QCOMPARE(left->m_targetID, KeyID{'('});
    QCOMPARE(left->m_targetButton, KeyButton{0x1D});
    QCOMPARE(left->m_targetModifiers, KeyModifierShift);
    const auto *right = calibration.find(0x1A, KeyModifierShift, ')');
    QVERIFY(right != nullptr);
    QCOMPARE(right->m_targetID, KeyID{')'});
    QVERIFY(calibration.find(0x1D, 0, '(') == nullptr);
    QCOMPARE(calibration.remapKeyID(kKeyF13), kKeyControl_L);
    QCOMPARE(calibration.remapKeyID(kKeyF14), kKeyF14);
  }
  void missingFileLeavesKeysUnchanged()
  {
    Log log;
    OSXKeyCalibration calibration("");
    QVERIFY(calibration.empty());
    QVERIFY(calibration.find(0x1D, KeyModifierShift, '(') == nullptr);
    QCOMPARE(calibration.remapKeyID(kKeyMuhenkan), kKeyMuhenkan);
  }
};
QTEST_MAIN(OSXKeyCalibrationTests)
#include "OSXKeyCalibrationTests.moc"
