// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include <QWidget>
#include <QStringList>
#include <functional>
namespace deskflow::gui {
class FileDropArea : public QWidget
{
  Q_OBJECT
public:
  FileDropArea(QWidget *parent, bool edge);
  std::function<void(const QStringList &)> armed, dropped;
protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dragMoveEvent(QDragMoveEvent *event) override;
  void dragLeaveEvent(QDragLeaveEvent *event) override;
  void dropEvent(QDropEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
private:
  bool m_edge, m_hover = false;
};
}
