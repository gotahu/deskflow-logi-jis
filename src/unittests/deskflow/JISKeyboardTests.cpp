// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "common/JISKeyboard.h"
#include <QTest>
class JISKeyboardTests : public QObject {
  Q_OBJECT
private Q_SLOTS:
  void shiftedSymbols() {
    QCOMPARE(deskflow::jis::symbol(0x1c, true), uint32_t('('));
    QCOMPARE(deskflow::jis::symbol(0x19, true), uint32_t(')'));
    QCOMPARE(deskflow::jis::symbol(0x13, true), uint32_t('"'));
    QCOMPARE(deskflow::jis::symbol(0x16, true), uint32_t('&'));
    QCOMPARE(deskflow::jis::symbol(0x27, true), uint32_t('*'));
    QCOMPARE(deskflow::jis::symbol(0x5e, true), uint32_t('_'));
  }
  void unshiftedSymbols() {
    QCOMPARE(deskflow::jis::symbol(0x21, false), uint32_t('@'));
    QCOMPARE(deskflow::jis::symbol(0x18, false), uint32_t('^'));
    QCOMPARE(deskflow::jis::symbol(0x1e, false), uint32_t('['));
    QCOMPARE(deskflow::jis::symbol(0x2a, false), uint32_t(']'));
    QCOMPARE(deskflow::jis::symbol(0x1c, false), uint32_t('8'));
  }
  void leavesOtherKeysOnExistingPath() {
    QCOMPARE(deskflow::jis::symbol(0x00, false), uint32_t(0)); // A
    QCOMPARE(deskflow::jis::symbol(0x66, false), uint32_t(0)); // Eisu
    QCOMPARE(deskflow::jis::symbol(0x68, false), uint32_t(0)); // Kana
    QCOMPARE(deskflow::jis::symbol(0x52, false), uint32_t(0)); // Keypad 0
  }
  void fixesLanguageIndependentlyOfIME() {
    QCOMPARE(deskflow::jis::language("en", true), std::string("ja"));
    QCOMPARE(deskflow::jis::language("ja", true), std::string("ja"));
    QCOMPARE(deskflow::jis::language("en", false), std::string("en"));
  }
};
QTEST_MAIN(JISKeyboardTests)
#include "JISKeyboardTests.moc"
