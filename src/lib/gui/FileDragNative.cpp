// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "FileDragNative.h"
#ifdef Q_OS_WIN
#include <Windows.h>
bool fileDragLeftDown() { return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0; }
#else
#include <QGuiApplication>
bool fileDragLeftDown() { return QGuiApplication::mouseButtons().testFlag(Qt::LeftButton); }
#endif
QStringList fileDragNativeFiles() { return {}; }
