// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once

#include "deskflow/IKeyState.h"
#include <algorithm>
#include <map>
#include <string>

namespace deskflow::windows {

inline bool isWindowsAppSession(std::wstring path)
{
  std::transform(path.begin(), path.end(), path.begin(), [](wchar_t c) {
    if (c == L'/') return L'\\';
    return c >= L'A' && c <= L'Z' ? static_cast<wchar_t>(c + L'a' - L'A') : c;
  });
  // Exclude the launcher, legacy Remote Desktop and WSLg's own msrdc.exe.
  return path.find(L"\\microsoftcorporationii.windows365_") != std::wstring::npos &&
         path.ends_with(L"\\msrdc.exe");
}

// Latch the destination of each Command key at key-down. KeyState releases
// by the original server button, even if focus changes before key-up.
class WindowsAppCommand
{
public:
  KeyID down(KeyID id, KeyButton button, bool active)
  {
    if (id == kKeySuper_L || id == kKeySuper_R) {
      const auto it = m_commands.try_emplace(normalize(button), active).first;
      return translated(id, it->second);
    }
    return id;
  }

  KeyID repeat(KeyID id, KeyButton button) const
  {
    const auto it = m_commands.find(normalize(button));
    return translated(id, it != m_commands.end() && it->second);
  }

  KeyModifierMask mask(KeyModifierMask value) const
  {
    if ((value & KeyModifierSuper) == 0) return value;
    bool ctrl = false, super = false;
    for (const auto &[button, mapped] : m_commands) {
      ctrl |= mapped;
      super |= !mapped;
    }
    if (ctrl) {
      value |= KeyModifierControl;
      if (!super) value &= ~KeyModifierSuper;
    }
    return value;
  }

  void up(KeyButton button) { m_commands.erase(normalize(button)); }
  void clear() { m_commands.clear(); }

private:
  static KeyButton normalize(KeyButton button) { return button & (IKeyState::s_numButtons - 1); }
  static KeyID translated(KeyID id, bool active)
  {
    if (active && id == kKeySuper_L) return kKeyControl_L;
    if (active && id == kKeySuper_R) return kKeyControl_R;
    return id;
  }
  std::map<KeyButton, bool> m_commands;
};
} // namespace deskflow::windows
