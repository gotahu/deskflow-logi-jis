// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <Windows.h>
#include <array>
#include <string>
namespace deskflow::windows {
inline std::string keyboardLanguage(HKL layout)
{
  if (!layout) {
    return {};
  }
  // GetLocaleInfoA needs room for the terminating NUL as well as the ISO code.
  std::array<char, 16> code{};
  const auto locale = MAKELCID(LOWORD(reinterpret_cast<ULONG_PTR>(layout)), SORT_DEFAULT);
  if (GetLocaleInfoA(locale, LOCALE_SISO639LANGNAME, code.data(), static_cast<int>(code.size())) == 0) {
    return {};
  }
  return code.data();
}
} // namespace deskflow::windows
