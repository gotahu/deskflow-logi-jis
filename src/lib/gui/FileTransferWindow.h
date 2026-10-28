// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#pragma once
#include "FileTransferHub.h"
#include "FileDragGesture.h"
#include "gui/core/CoreProcess.h"
#include <QElapsedTimer>
#include <QWidget>
class QComboBox;
class QLabel;
class QProgressBar;
class QListWidget;
class QCheckBox;
namespace deskflow::gui {
class FileDropArea;
class FileTransferWindow : public QWidget
{
  Q_OBJECT
public:
  explicit FileTransferWindow(CoreProcess *core, QWidget *parent = nullptr);
  void dropFiles(const QStringList &paths);
protected:
  void dragEnterEvent(QDragEnterEvent *event) override;
  void dropEvent(QDropEvent *event) override;
private:
  void configure();
  void scan();
  void arm(const QStringList &paths);
  void clearDrag();
  void input(const QString &kind, const QString &target);
  void finishDrag(const QString &target);
  void networkControl(const QString &peer, const QJsonObject &o);
  void updatePeers();
  void addHistory(const QString &text);
  CoreProcess *m_core;
  QString m_downloads, m_activeName;
  FileTransferHub *m_hub;
  QComboBox *m_peer;
  QLabel *m_status, *m_preview;
  QProgressBar *m_progress;
  QListWidget *m_history;
  QCheckBox *m_enabled, *m_cross;
  QTimer m_scan, m_configure;
  QList<FileDropArea *> m_edges;
  FileDragGesture m_edgeGesture;
  QStringList m_dragPaths;
  QString m_remoteDrag;
  QElapsedTimer m_dragAge, m_pulse;
};
}
