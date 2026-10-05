/*
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */
#pragma once
#include <cstdint>

namespace deskflow {

// KeyButton uses bit 8 for E0. Some mouse shortcut generators supply only
// a virtual key; without E0 the arrows become keypad navigation on clients.
constexpr uint32_t normalizeInjectedArrowScanCode(
    uint32_t virtualKey, uint32_t scanCode, bool extended, bool injected
)
{
  if (injected && scanCode == 0) {
    switch (virtualKey) {
    case 0x25: // VK_LEFT
      return 0x14b;
    case 0x26: // VK_UP
      return 0x148;
    case 0x27: // VK_RIGHT
      return 0x14d;
    case 0x28: // VK_DOWN
      return 0x150;
    default:
      break;
    }
  }
  return scanCode | (extended ? 0x100u : 0u);
}

} // namespace deskflow
