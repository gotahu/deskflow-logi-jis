// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "FileDropArea.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QUrl>
namespace deskflow::gui {
namespace {
QStringList localFiles(const QMimeData *mime) {
  QStringList paths;
  for (const auto &url : mime->urls()) if (url.isLocalFile()) paths.append(url.toLocalFile());
  return paths;
}
}
FileDropArea::FileDropArea(QWidget *parent, bool edge)
    : QWidget(parent, edge ? Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint |
          Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus : Qt::Widget), m_edge(edge)
{
  setAcceptDrops(true);
  setAttribute(Qt::WA_ShowWithoutActivating, edge);
  setAttribute(Qt::WA_MacAlwaysShowToolWindow, edge);
  setAttribute(Qt::WA_TranslucentBackground, edge);
  setAccessibleName(tr("ファイルをドロップして相手へ送信"));
  if (!edge) setMinimumHeight(120);
}
void FileDropArea::dragEnterEvent(QDragEnterEvent *e) {
  auto paths = localFiles(e->mimeData());
  if (paths.isEmpty()) return;
  e->setDropAction(Qt::CopyAction); e->accept();
  m_hover = true; update();
  if (armed) armed(paths);
}
void FileDropArea::dragMoveEvent(QDragMoveEvent *e) { e->setDropAction(Qt::CopyAction); e->accept(); }
void FileDropArea::dragLeaveEvent(QDragLeaveEvent *e) { m_hover = false; update(); e->accept(); }
void FileDropArea::dropEvent(QDropEvent *e) {
  auto paths = localFiles(e->mimeData());
  if (paths.isEmpty()) return;
  e->setDropAction(Qt::CopyAction); e->accept();
  m_hover = false; update();
  if (dropped) dropped(paths);
}
void FileDropArea::paintEvent(QPaintEvent *) {
  QPainter p(this);
  if (m_edge) {
    // Windows skips zero-alpha pixels during OLE hit testing. Keep only the
    // minimum nonzero alpha, with no border, hover highlight or native shadow.
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(rect(), QColor(128,128,128,1));
    return;
  }
  const auto color = palette().color(m_hover ? QPalette::Highlight : QPalette::Mid);
  p.fillRect(rect(), palette().brush(m_hover ? QPalette::Highlight : QPalette::Base));
  p.setPen(color); p.drawRect(rect().adjusted(0,0,-1,-1));
  p.setPen(palette().color(m_hover ? QPalette::HighlightedText : QPalette::Text));
  p.drawText(rect().adjusted(12,12,-12,-12), Qt::AlignCenter | Qt::TextWordWrap,
             tr("ここにファイルをドロップ\n接続先のダウンロードに保存します"));
}
}
