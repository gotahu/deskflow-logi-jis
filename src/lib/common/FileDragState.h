// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <atomic>
#include <chrono>
namespace deskflow {
inline std::atomic<long long> fileDragUntil{0};
inline long long fileDragNow() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline void armFileDrag(bool armed) { fileDragUntil.store(armed ? fileDragNow() + 2500 : 0); }
inline bool fileDragArmed() { return fileDragNow() < fileDragUntil.load(); }
}
