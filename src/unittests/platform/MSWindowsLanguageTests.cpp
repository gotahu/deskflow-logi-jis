// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/WindowsKeyboardLanguage.h"
#include <QTest>
class MSWindowsLanguageTests : public QObject {
  Q_OBJECT
private Q_SLOTS:
  void languageCodesIncludeTerminatorSpace() {
    QCOMPARE(deskflow::windows::keyboardLanguage(reinterpret_cast<HKL>(0x00000411)), std::string("ja"));
    QCOMPARE(deskflow::windows::keyboardLanguage(reinterpret_cast<HKL>(0x00000409)), std::string("en"));
  }
  void imeHandlesUseLanguageIdentifier() {
    QCOMPARE(deskflow::windows::keyboardLanguage(reinterpret_cast<HKL>(0xe0200411)), std::string("ja"));
  }
  void missingLayoutHasNoCode() {
    QVERIFY(deskflow::windows::keyboardLanguage(nullptr).empty());
  }
};
QTEST_MAIN(MSWindowsLanguageTests)
#include "MSWindowsLanguageTests.moc"
