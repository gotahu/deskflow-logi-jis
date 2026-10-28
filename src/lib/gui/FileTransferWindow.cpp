// SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
// SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
#include "FileTransferWindow.h"
#include "FileDragNative.h"
#include "FileDropArea.h"
#include "common/Settings.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QLabel>
#include <QListWidget>
#include <QMimeData>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace deskflow::gui {
namespace {
QStringList localFiles(const QMimeData *mime) {
  QStringList paths;
  if (!mime->hasUrls()) return paths;
  for (const auto &url : mime->urls()) if (url.isLocalFile()) paths.append(url.toLocalFile());
  return paths;
}
}
FileTransferWindow::FileTransferWindow(CoreProcess *core, QWidget *parent)
    : QWidget(parent, Qt::Window), m_core(core),
      m_downloads(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)),
      m_hub(new FileTransferHub(m_downloads, this)), m_peer(new QComboBox(this)),
      m_status(new QLabel(tr("接続を確認しています…"), this)), m_preview(new QLabel(nullptr, Qt::ToolTip)),
      m_progress(new QProgressBar(this)), m_history(new QListWidget(this)),
      m_enabled(new QCheckBox(tr("ファイル転送を有効にする"), this)),
      m_cross(new QCheckBox(tr("画面をまたぐドラッグ＆ドロップ"), this))
{
  setWindowTitle(tr("Deskflow — ファイル転送"));
  setMinimumSize(440, 560);
  resize(500, 620);
  setAcceptDrops(true);
  QDir().mkpath(m_downloads);
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(20,20,20,20);
  layout->setSpacing(10);
  auto *title = new QLabel(tr("ファイル転送"), this);
  auto font = title->font(); font.setPointSize(font.pointSize() + 4); font.setBold(true); title->setFont(font);
  auto *heading = new QHBoxLayout;
  heading->addWidget(title); heading->addStretch(); heading->addWidget(m_enabled);
  layout->addLayout(heading);
  auto *sendRow = new QHBoxLayout;
  sendRow->addWidget(new QLabel(tr("送信先"), this));
  m_peer->setAccessibleName(tr("ファイルの送信先"));
  m_peer->setPlaceholderText(tr("接続先を待っています"));
  sendRow->addWidget(m_peer, 1);
  layout->addLayout(sendRow);
  auto *area = new FileDropArea(this, false);
  area->dropped = [this](const QStringList &paths) { dropFiles(paths); };
  layout->addWidget(area);
  auto *choose = new QPushButton(tr("ファイルを選択…"), this);
  connect(choose, &QPushButton::clicked, this, [this] { dropFiles(QFileDialog::getOpenFileNames(this, tr("送信するファイル"))); });
  sendRow->addWidget(choose);
  layout->addWidget(m_cross);
  auto *destination = new QLabel(tr("受信先: %1").arg(QDir::toNativeSeparators(m_downloads)), this);
  destination->setWordWrap(true); destination->setTextInteractionFlags(Qt::TextSelectableByMouse);
  auto *receiveRow = new QHBoxLayout;
  receiveRow->addWidget(destination, 1);
  auto *open = new QPushButton(tr("ダウンロードを開く"), this);
  connect(open, &QPushButton::clicked, this, [this] { QDesktopServices::openUrl(QUrl::fromLocalFile(m_downloads)); });
  receiveRow->addWidget(open);
  layout->addLayout(receiveRow);
  m_status->setWordWrap(true);
  m_status->setTextFormat(Qt::PlainText);
  layout->addWidget(m_status);
  m_progress->setRange(0,100); m_progress->setValue(0); layout->addWidget(m_progress);
  m_history->setMinimumHeight(72);
  layout->addWidget(m_history, 1);
  auto *cancel = new QPushButton(tr("転送を中止"), this);
  cancel->setEnabled(false);
  auto updateCancel = [this, cancel] { cancel->setEnabled(m_hub->busy()); };
  connect(m_hub, &FileTransferHub::activityChanged, this, updateCancel);
  connect(m_hub, &FileTransferHub::peersChanged, this, updateCancel);
  connect(cancel, &QPushButton::clicked, this, [this] { clearDrag(); m_hub->cancel(); });
  layout->addWidget(cancel);
  m_preview->setMargin(10); m_preview->setTextFormat(Qt::PlainText);
  m_preview->setAttribute(Qt::WA_ShowWithoutActivating);
  m_preview->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_enabled->setChecked(Settings::value(Settings::Files::Enabled).toBool());
  m_cross->setChecked(Settings::value(Settings::Files::CrossScreen).toBool());
  connect(m_enabled, &QCheckBox::toggled, this, [this](bool b) {
    Settings::setValue(Settings::Files::Enabled,b); clearDrag(); configure();
  });
  connect(m_cross, &QCheckBox::toggled, this, [this](bool b) {
    Settings::setValue(Settings::Files::CrossScreen,b); if (!b) clearDrag();
  });
  connect(m_hub, &FileTransferHub::status, m_status, &QLabel::setText);
  connect(m_hub, &FileTransferHub::failed, this, [this](const QString &text) {
    m_status->setText(text); addHistory(tr("失敗: %1").arg(text));
  });
  connect(m_hub, &FileTransferHub::progress, this, [this](qint64 done, qint64 total) {
    m_progress->setValue(total == 0 ? 100 : static_cast<int>(100.0 * done / total));
  });
  connect(m_hub, &FileTransferHub::received, this, [this](const QString &path) {
    m_status->setText(tr("受信完了: %1").arg(QFileInfo(path).fileName()));
    addHistory(tr("受信: %1").arg(QFileInfo(path).fileName())); m_progress->setValue(100);
  });
  connect(m_hub, &FileTransferHub::sent, this, [this](const QString &name) {
    m_status->setText(tr("送信完了: %1").arg(name)); addHistory(tr("送信: %1").arg(name));
  });
  connect(m_hub, &FileTransferHub::peersChanged, this, &FileTransferWindow::updatePeers);
  connect(m_hub, &FileTransferHub::controlReceived, this, &FileTransferWindow::networkControl);
  connect(core, &CoreProcess::fileInputEvent, this, &FileTransferWindow::input);
  connect(core, &CoreProcess::processStateChanged, this, [this] { configure(); });
  m_scan.setInterval(50); connect(&m_scan, &QTimer::timeout, this, &FileTransferWindow::scan); m_scan.start();
  m_configure.setInterval(5000); connect(&m_configure, &QTimer::timeout, this, &FileTransferWindow::configure); m_configure.start();
  connect(this, &QObject::destroyed, m_preview, &QObject::deleteLater);
  m_activeName = Settings::value(Settings::Core::ComputerName).toString();
  configure();
}
void FileTransferWindow::configure()
{
  const bool server = m_core->mode() == Settings::CoreMode::Server;
  const auto host = Settings::value(Settings::Client::RemoteHost).toString();
  const QUrl url(QStringLiteral("deskflow://") + host);
  const auto basePort = server ? Settings::value(Settings::Core::Port).toInt() :
                                url.port(Settings::value(Settings::Core::Port).toInt());
  if (server != m_hub->serverMode() || m_hub->localName() != Settings::value(Settings::Core::ComputerName).toString())
    m_activeName = Settings::value(Settings::Core::ComputerName).toString();
  m_hub->configure(m_enabled->isChecked() && m_core->isStarted() &&
                  Settings::value(Settings::Security::TlsEnabled).toBool(), server,
                  url.host(), static_cast<quint16>(basePort >= 65535 ? 24801 : basePort + 1),
                  Settings::value(Settings::Core::ComputerName).toString(),
                  Settings::value(Settings::Security::Certificate).toString(),
                  server ? Settings::tlsTrustedClientsDb() : Settings::tlsTrustedServersDb(),
                  server ? Settings::value(Settings::Core::Interface).toString() : QString());
}
void FileTransferWindow::updatePeers()
{
  auto selected = m_peer->currentText();
  m_peer->clear(); m_peer->addItems(m_hub->peers());
  if (m_peer->findText(selected) >= 0) m_peer->setCurrentText(selected);
  if (m_hub->peers().isEmpty()) { clearDrag(); m_status->setText(tr("相手のファイル転送への接続を待っています。")); }
  if (m_hub->serverMode()) m_hub->broadcast({{"event","screen"},{"target",m_activeName}});
}
void FileTransferWindow::dropFiles(const QStringList &paths)
{
  if (paths.isEmpty()) return;
  m_hub->sendFiles(m_peer->currentText(), paths);
}
void FileTransferWindow::dragEnterEvent(QDragEnterEvent *e) {
  if (!localFiles(e->mimeData()).isEmpty()) { e->setDropAction(Qt::CopyAction); e->accept(); }
}
void FileTransferWindow::dropEvent(QDropEvent *e) {
  auto paths = localFiles(e->mimeData());
  if (!paths.isEmpty()) { e->setDropAction(Qt::CopyAction); e->accept(); dropFiles(paths); }
}
void FileTransferWindow::addHistory(const QString &text) {
  m_history->insertItem(0, text);
  while (m_history->count() > 100) delete m_history->takeItem(m_history->count()-1);
}
void FileTransferWindow::arm(const QStringList &paths)
{
  if (!m_cross->isChecked() || m_hub->peers().isEmpty() || m_activeName != m_hub->localName()) return;
  QStringList valid;
  for (const auto &path : paths) if (QFileInfo(path).isFile()) valid.append(path);
  if (valid.isEmpty() || valid.size() > 1024) return;
  m_dragPaths = valid;
  m_dragAge.restart(); m_pulse.invalidate();
}
void FileTransferWindow::clearDrag()
{
  m_dragPaths.clear(); m_remoteDrag.clear();
  m_preview->hide(); m_core->armFileDrag(false);
}
void FileTransferWindow::input(const QString &kind, const QString &target)
{
  if (!m_hub->serverMode()) return;
  if (kind == "Screen") m_activeName = target;
  if (kind == "Press" || kind == "Cancel") clearDrag();
  if (kind == "Drop") { finishDrag(target); m_remoteDrag.clear(); m_preview->hide(); }
  m_hub->broadcast({{"event",kind.toLower()},{"target",target}});
}
void FileTransferWindow::networkControl(const QString &peer, const QJsonObject &o)
{
  const auto event = o.value("event").toString();
  if (event == "arm") {
    if (!m_cross->isChecked()) return;
    if (m_hub->serverMode()) {
      if (!fileDragLeftDown()) return;
      if (m_activeName == peer || !m_remoteDrag.isEmpty()) m_core->armFileDrag(o.value("armed").toBool());
    }
    m_remoteDrag = o.value("name").toString().left(200);
    return;
  }
  // Only the keyboard/mouse server may author pointer/drop events.
  if (m_hub->serverMode()) return;
  if (event == "screen") m_activeName = o.value("target").toString();
  if (event == "press" || event == "cancel") clearDrag();
  if (event == "drop") { finishDrag(o.value("target").toString()); m_remoteDrag.clear(); m_preview->hide(); }
}
void FileTransferWindow::finishDrag(const QString &target)
{
  const auto paths = m_dragPaths;
  const bool valid = m_dragAge.isValid() && m_dragAge.elapsed() < 60000;
  clearDrag();
  if (!paths.isEmpty() && valid && target != m_hub->localName()) m_hub->sendFiles(target, paths);
}
void FileTransferWindow::scan()
{
  const auto native = m_enabled->isChecked() && m_cross->isChecked() && !m_hub->peers().isEmpty()
                          ? fileDragNativeFiles() : QStringList();
  if (!native.isEmpty()) arm(native);
  if (!m_dragPaths.isEmpty()) {
    if (m_dragAge.elapsed() >= 60000) clearDrag();
    else if (!m_pulse.isValid() || m_pulse.elapsed() > 700) {
      if (m_hub->serverMode()) m_core->armFileDrag(true);
      m_hub->broadcast({{"event","arm"},{"armed",true},{"name",QFileInfo(m_dragPaths.first()).fileName()}});
      m_pulse.restart();
    }
  }
  const bool canDetectDrag = m_cross->isChecked() && m_enabled->isChecked() &&
                         !m_hub->peers().isEmpty() && m_activeName == m_hub->localName();
  const bool dragging = m_edgeGesture.update(canDetectDrag, fileDragLeftDown(), QCursor::pos(),
                                             QApplication::startDragDistance());
  if (dragging && m_edges.isEmpty()) {
    for (auto *screen : QGuiApplication::screens()) {
      const auto g = screen->geometry();
      for (const auto r : {QRect(g.left(),g.top(),g.width(),16), QRect(g.left(),g.bottom()-15,g.width(),16),
                           QRect(g.left(),g.top()+16,16,g.height()-32), QRect(g.right()-15,g.top()+16,16,g.height()-32)}) {
        auto *edge = new FileDropArea(this,true); edge->setGeometry(r);
        edge->armed = [this](const QStringList &paths) { arm(paths); };
        // The core mouse-up event decides whether the pointer crossed to the peer.
        // A local edge drop alone must not send or duplicate the files.
        m_edges.append(edge);
      }
    }
  }
  for (auto *edge : m_edges) edge->setVisible(dragging);
  if (!m_remoteDrag.isEmpty() && m_activeName == m_hub->localName() && m_cross->isChecked()) {
    m_preview->setText(tr("%1\n離すとダウンロードに保存").arg(m_remoteDrag));
    m_preview->adjustSize(); m_preview->move(QCursor::pos()+QPoint(18,18)); m_preview->show();
  } else m_preview->hide();
}
}
