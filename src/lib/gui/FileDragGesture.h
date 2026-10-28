// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QPoint>
namespace deskflow::gui {
// Polling must not create edge windows for stationary clicks or a held button.
class FileDragGesture
{
public:
  bool update(bool enabled, bool down, const QPoint &position, int distance) {
    if (!enabled || !down) {
      m_down = m_dragging = false;
      return false;
    }
    if (!m_down) {
      m_origin = position;
      m_down = true;
    } else if ((position - m_origin).manhattanLength() >= distance) {
      m_dragging = true;
    }
    return m_dragging;
  }
private:
  QPoint m_origin;
  bool m_down = false, m_dragging = false;
};
}
