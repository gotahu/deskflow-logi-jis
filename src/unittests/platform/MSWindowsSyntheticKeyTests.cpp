/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#include "platform/MSWindowsSyntheticKey.h"
#include <QTest>

class MSWindowsSyntheticKeyTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void normalize_data()
  {
    QTest::addColumn<quint32>("vk");
    QTest::addColumn<quint32>("scan");
    QTest::addColumn<bool>("extended");
    QTest::addColumn<bool>("injected");
    QTest::addColumn<quint32>("expected");
    QTest::newRow("left-shortcut") << 0x25u << 0u << false << true << 0x14bu;
    QTest::newRow("up-shortcut") << 0x26u << 0u << false << true << 0x148u;
    QTest::newRow("right-shortcut") << 0x27u << 0u << false << true << 0x14du;
    QTest::newRow("down-shortcut") << 0x28u << 0u << false << true << 0x150u;
    QTest::newRow("missing-scan-with-e0") << 0x25u << 0u << true << true << 0x14bu;
    QTest::newRow("physical-arrow") << 0x25u << 0x4bu << true << false << 0x14bu;
    QTest::newRow("physical-keypad") << 0x25u << 0x4bu << false << false << 0x4bu;
    QTest::newRow("injected-keypad") << 0x25u << 0x4bu << false << true << 0x4bu;
    QTest::newRow("non-injected-missing-scan") << 0x25u << 0u << false << false << 0u;
    QTest::newRow("other-shortcut") << 0x41u << 0u << false << true << 0u;
  }
  void normalize()
  {
    QFETCH(quint32, vk);
    QFETCH(quint32, scan);
    QFETCH(bool, extended);
    QFETCH(bool, injected);
    QFETCH(quint32, expected);
    QCOMPARE(deskflow::normalizeInjectedArrowScanCode(vk, scan, extended, injected), expected);
  }
};
QTEST_APPLESS_MAIN(MSWindowsSyntheticKeyTests)
#include "MSWindowsSyntheticKeyTests.moc"
