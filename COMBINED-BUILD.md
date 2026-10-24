# Deskflow Logi navigation + JIS combined build

Upstream base: Deskflow v1.27.0, commit 081f6478e.
Release: https://github.com/deskflow/deskflow/releases/tag/v1.27.0

Preserved upstream keyboard-type translation and settings migration.

Sources:
- https://github.com/andy5090/deskflow-andy-custom/tree/macos-logi-options-plus
  Imported mouse fork: fb97e555baad7b4d23543a0c0e925fdbc77d3621
- https://github.com/tokunagakazuya/deskflow/tree/v1.26.0-jis
  Imported: c60c4bc98dc134c362f9af88b3c16632d0bd48a9 and 7fcedaf3b818

Includes macOS navigation gesture forwarding for Logi Options+, JIS IME
keys, F17-F20, right modifiers, and Help > Keyboard Calibration.

Integration adjustments:
- Retain the mouse fork's ASCII input source and remote Caps Lock handling.
- Adapt navigation settings and handlers to upstream Computer classes and
  updated Save/Reset/Restore Defaults behavior.
- Normalize injected Windows arrows with missing scan codes to E0 arrows;
  preserve ordinary keyboard/keypad events. Windows client injected-event
  filtering and Deskflow self-injection suppression remain in place.
- Keep both left and right modifier flags when both are held.
- Retain Windows VK_KANA and map IME_ON/OFF to native macOS Kana/Eisu.
- Reconcile each modifier using its own physical key code.
- Adapt the calibration logging to the mouse fork's current logging API.
- Add tests for JIS native key mappings, both modifier sides, and calibration
  file loading (including shifted parentheses and key overrides).

## Use

Mac: quit the currently running Deskflow, then launch this build.
For Mac server mouse navigation: Configure Server > Advanced >
Forward macOS navigation gestures. Use Detect for the desired actions.

Windows server IME keys also require the JIS changes on Windows.
This source includes them. This local build contains only a macOS arm64
binary; Windows has not been built or tested here. The author's published
v1.26.0-jis Windows release has the JIS-side changes and uses the same
Deskflow protocol, but pairing it with this build needs a device test.
That published Windows release does not contain the new synthetic arrow
normalization: this integrated Windows source must be built for that fix.

Symbol correction: Help > Keyboard Calibration on the Mac client.
Capture the desired key with a keyboard directly connected to the Mac,
then capture the same key through Deskflow from Windows. Save each mapping
and restart the Deskflow core to reload keyboard-calibration.json.
Correct each affected symbol/shift combination separately.
Windows and Mac IME state remains independent; this forwards switching
key presses and does not synchronize the two IME engines' internal state.

## Build

Requires CMake, Qt 6.10.3, OpenSSL 3 and platform compiler.
macOS example (set Qt/OpenSSL paths to your installations):

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DSKIP_BUILD_TESTS=ON \
  -DCMAKE_PREFIX_PATH=/path/to/qt/6.10.3/macos \
  -DDEPLOYQT=/path/to/qt/6.10.3/macos/bin/macdeployqt \
  -DOPENSSL_ROOT_DIR=/path/to/openssl \
  -DCMAKE_INSTALL_PREFIX=/path/to/output
cmake --build build --parallel 8
cmake --install build
codesign --force --deep --sign - /path/to/output/Deskflow.app
```

A manual Windows x64 GitHub Actions workflow is included in
.github/workflows/custom-build.yml. It has not been run or verified here.

## macOS scroll direction capture

Scroll forwarding now uses a separate annotated-session event tap, after
session-level mouse utilities apply direction corrections. Local Mac scrolling
passes through unchanged. Keyboard, buttons and movement retain the original
HID tap. If the later tap cannot be created, the previous HID scroll path is
retained with a warning. Physical trackpad/Logi behavior needs device testing.

## Trackpad vertical scroll speed

Mac Settings > Advanced includes Trackpad vertical scroll speed when forwarding.
The default is 25%; 100% retains the previous speed. This applies only to events
with scroll or momentum phases. Continuous wheel events without gesture phases
keep the existing mouse speed. Horizontal and local scrolling are unchanged.
Fractional scaled deltas accumulate to preserve slow, small movements.
Some third-party drivers may attach gesture phases to wheel events; device
classification and the preferred percentage still require a physical test.

Trackpad vertical direction can also be reversed independently in the Mac
Advanced settings. This custom build defaults the checkbox to ON to compensate
for the reported trackpad/wheel mismatch while retaining the client's existing
Invert setting. Disable it if no compensation is needed in your setup.

## Fixed JIS keyboard mode (r3)

Mac Settings > Advanced > Send keys using the Japanese physical keyboard layout
(JIS) defaults ON in this custom Mac build. Symbol positions and the transmitted
language are independent of the current Mac IME. Eisu/Kana and ordinary letter,
modifier and keypad processing retain their existing paths.

Windows Configure Client > Always use the Japanese keyboard layout (JIS)
defaults ON. This selects the installed Japanese layout regardless of en/ja
metadata from the server and takes precedence over normal language sync.
ATOK/IME on/off remains controlled by the IME keys. The Japanese keyboard
layout must be installed in Windows. Turn these options OFF for US keyboards.

Mac and Windows should both be updated for this change. Existing scroll speed
and direction settings remain unchanged.

Windows language detection also fixes an undersized GetLocaleInfoA buffer.
Japanese, English and Japanese IME handles now return usable ISO language codes;
a Windows-specific regression test validates the actual Windows API results.

When several input methods share Japanese, the active Japanese IME is retained
(e.g. ATOK is not replaced with Microsoft IME just because both report ja).

## Windows App: Command → Ctrl (r4)

Windows クライアントでは、受信した Command（Super）を Windows App のリモート接続画面内だけ Ctrl に変換します。Mac サーバーは r3 のまま使用できます。

- Configure Client → **Use Command as Ctrl in Windows App remote sessions**（初期値 ON）。設定ファイルは `[client] windowsAppCommandToCtrl=true`。
- Windows App パッケージ `MicrosoftCorporationII.Windows365_*` 内の `msrdc.exe` が前面にある場合だけ有効です。Windows App の一覧画面、ブラウザー、従来の mstsc、WSLg の msrdc は対象外です。
- 左右の Command に対応します。押した時点の変換先を離すまで維持し、切断時は状態を破棄します。アプリを切り替えた後は Command を一度離して押し直してください。
- サーバーのコンピューター設定の **Super は Super のまま**にします。そこで Ctrl に変換すると、アプリを問わず Ctrl になります。
- 物理的に Windows に接続したキーボードは対象外です。受信した Super に OS の識別情報はないため、Mac 以外のサーバーから受信した Super も、この設定が ON なら同じ変換対象になります。

Windows App のプロセス構成の根拠: https://github.com/Azure/WindowsAppKiosk/blob/main/source/WindowsApp/ADVANCED_CUSTOMIZATIONS.md
