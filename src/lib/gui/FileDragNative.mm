// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "FileDragNative.h"
#import <Cocoa/Cocoa.h>
bool fileDragLeftDown() { return ([NSEvent pressedMouseButtons] & 1) != 0; }
QStringList fileDragNativeFiles()
{
  NSPasteboard *board = [NSPasteboard pasteboardWithName:NSPasteboardNameDrag];
  static NSInteger previous = [board changeCount];
  if (!fileDragLeftDown()) { previous = [board changeCount]; return {}; }
  if (previous == [board changeCount]) return {};
  previous = [board changeCount];
  NSArray *urls = [board readObjectsForClasses:@[[NSURL class]]
                                      options:@{NSPasteboardURLReadingFileURLsOnlyKey: @YES}];
  QStringList paths;
  for (NSURL *url in urls) {
    if ([url isFileURL]) paths.append(QString::fromUtf8([[url path] UTF8String]));
  }
  return paths;
}
