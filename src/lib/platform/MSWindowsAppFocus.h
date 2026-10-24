// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "platform/WindowsAppCommand.h"
#include <Windows.h>
#include <vector>

namespace deskflow::windows {
inline bool isWindowsAppForeground()
{
  const HWND window = GetForegroundWindow();
  DWORD pid = 0;
  if (!window || !GetWindowThreadProcessId(window, &pid)) return false;
  const HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
  if (!process) return false;
  std::vector<wchar_t> path(32768);
  DWORD length = static_cast<DWORD>(path.size());
  const bool found = QueryFullProcessImageNameW(process, 0, path.data(), &length) != FALSE;
  CloseHandle(process);
  return found && isWindowsAppSession(std::wstring(path.data(), length));
}
} // namespace deskflow::windows
