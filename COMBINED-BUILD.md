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
