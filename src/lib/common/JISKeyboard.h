// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace deskflow::jis {
// macOS virtual key codes are physical positions, independent of the active IME.
struct Symbol { uint32_t code; char plain; char shifted; };
inline constexpr std::array symbols = {
    Symbol{0x12, '1', '!'}, Symbol{0x13, '2', '"'}, Symbol{0x14, '3', '#'},
    Symbol{0x15, '4', '$'}, Symbol{0x17, '5', '%'}, Symbol{0x16, '6', '&'},
    Symbol{0x1a, '7', '\''}, Symbol{0x1c, '8', '('}, Symbol{0x19, '9', ')'},
    Symbol{0x1d, '0', '0'}, Symbol{0x1b, '-', '='}, Symbol{0x18, '^', '~'},
    Symbol{0x21, '@', '`'}, Symbol{0x1e, '[', '{'}, Symbol{0x29, ';', '+'},
    Symbol{0x27, ':', '*'}, Symbol{0x2a, ']', '}'}, Symbol{0x2b, ',', '<'},
    Symbol{0x2f, '.', '>'}, Symbol{0x2c, '/', '?'},
    Symbol{0x5d, '\\', '|'}, Symbol{0x5e, '\\', '_'}
};
constexpr uint32_t symbol(uint32_t code, bool shifted)
{
  for (const auto &entry : symbols) {
    if (entry.code == code) {
      return static_cast<unsigned char>(shifted ? entry.shifted : entry.plain);
    }
  }
  return 0; // Letters, modifiers, IME keys and keypad retain their existing path.
}
inline std::string language(const std::string &actual, bool fixed)
{
  return fixed ? "ja" : actual;
}
} // namespace deskflow::jis
