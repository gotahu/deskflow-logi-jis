// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "platform/WindowsAppCommand.h"
#include <QTest>

class WindowsAppCommandTests : public QObject
{
  Q_OBJECT
private Q_SLOTS:
  void sessionDetection()
  {
    using deskflow::windows::isWindowsAppSession;
    QVERIFY(isWindowsAppSession(L"C:\\Program Files\\WindowsApps\\MicrosoftCorporationII.Windows365_2.0_x64__8wekyb3d8bbwe\\msrdc\\msrdc.exe"));
    QVERIFY(isWindowsAppSession(L"C:/Program Files/WindowsApps/MICROSOFTCORPORATIONII.WINDOWS365_3.0_ARM64__8wekyb3d8bbwe/msrdc/MSRDC.EXE"));
    QVERIFY(!isWindowsAppSession(L"C:\\Windows\\System32\\mstsc.exe"));
    QVERIFY(!isWindowsAppSession(L"C:\\Program Files\\WSL\\msrdc.exe"));
    QVERIFY(!isWindowsAppSession(L"C:\\Program Files\\WindowsApps\\MicrosoftCorporationII.Windows365_2.0\\Windows365.exe"));
    QVERIFY(!isWindowsAppSession(L"C:\\Program Files\\WindowsApps\\MicrosoftCorporationII.Windows365_2.0\\msedgewebview2.exe"));
    QVERIFY(!isWindowsAppSession(L""));
  }

  void copyPasteAndFocusChange()
  {
    deskflow::windows::WindowsAppCommand mapping;
    QCOMPARE(mapping.down(kKeySuper_L, 55, true), KeyID(kKeyControl_L));
    QCOMPARE(mapping.down('c', 8, false), KeyID('c'));
    QCOMPARE(mapping.mask(KeyModifierSuper | KeyModifierShift), KeyModifierMask(KeyModifierControl | KeyModifierShift));
    // Focus has changed: held keys and repeat must keep their destination.
    QCOMPARE(mapping.down(kKeySuper_L, 55, false), KeyID(kKeyControl_L));
    QCOMPARE(mapping.repeat(kKeySuper_L, 55), KeyID(kKeyControl_L));
    QCOMPARE(mapping.repeat('v', 9), KeyID('v'));
    QCOMPARE(mapping.mask(KeyModifierSuper), KeyModifierMask(KeyModifierControl));
    mapping.up(55);
    QCOMPARE(mapping.down(kKeySuper_L, 55, false), KeyID(kKeySuper_L));
    QCOMPARE(mapping.mask(KeyModifierSuper), KeyModifierMask(KeyModifierSuper));
  }

  void outsideSessionStaysWindowsKeyUntilReleased()
  {
    deskflow::windows::WindowsAppCommand mapping;
    QCOMPARE(mapping.down(kKeySuper_R, 54, false), KeyID(kKeySuper_R));
    QCOMPARE(mapping.down(kKeySuper_R, 54, true), KeyID(kKeySuper_R));
    QCOMPARE(mapping.mask(KeyModifierSuper | KeyModifierAlt), KeyModifierMask(KeyModifierSuper | KeyModifierAlt));
    mapping.up(54);
    QCOMPARE(mapping.down(kKeySuper_R, 54, true), KeyID(kKeyControl_R));
  }

  void bothSidesAndDisconnect()
  {
    deskflow::windows::WindowsAppCommand mapping;
    mapping.down(kKeySuper_L, 55, true);
    mapping.down(kKeySuper_R, 54, true);
    mapping.up(55);
    QCOMPARE(mapping.mask(KeyModifierSuper | KeyModifierControl), KeyModifierMask(KeyModifierControl));
    mapping.up(54);
    QCOMPARE(mapping.mask(KeyModifierControl), KeyModifierMask(KeyModifierControl));
    mapping.down(kKeySuper_L, 55, true);
    mapping.clear();
    QCOMPARE(mapping.mask(KeyModifierSuper), KeyModifierMask(KeyModifierSuper));
    QCOMPARE(mapping.down(kKeySuper_L, 55, false), KeyID(kKeySuper_L));
  }

  void differentlyLatchedSides()
  {
    deskflow::windows::WindowsAppCommand mapping;
    mapping.down(kKeySuper_L, 55, true);
    mapping.down(kKeySuper_R, 54, false);
    QCOMPARE(mapping.mask(KeyModifierSuper), KeyModifierMask(KeyModifierSuper | KeyModifierControl));
    mapping.up(54);
    QCOMPARE(mapping.mask(KeyModifierSuper), KeyModifierMask(KeyModifierControl));
    QCOMPARE(mapping.mask(0), KeyModifierMask(0));
  }
};
QTEST_APPLESS_MAIN(WindowsAppCommandTests)
#include "WindowsAppCommandTests.moc"
