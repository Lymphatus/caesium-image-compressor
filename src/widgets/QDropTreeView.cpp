#include "QDropTreeView.h"

#include <QDragEnterEvent>
#include <QMimeData>

#include <QDebug>
#include <QFileInfo>

QDropTreeView::QDropTreeView(QWidget* parent)
    : QTreeView(parent)
{
}

void QDropTreeView::dragEnterEvent(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void QDropTreeView::dragMoveEvent(QDragMoveEvent* event)
{
    event->accept();
}

void QDropTreeView::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();
    QList<QUrl> urlList = mimeData->urls();
    QStringList fileList;
    if (mimeData->hasFormat("text/uri-list")) {
        foreach (QUrl url, urlList) {
            QString absolutePath = url.toLocalFile();
            //TODO This is limitation by now. We only accept drops for files. May be changed in the future.
            if (QFileInfo(absolutePath).isFile()) {
                fileList << url.toLocalFile();
            } else if (QFileInfo(absolutePath).isDir()) {
                return;
            }
        }
    }

    event->acceptProposedAction();
    emit dropFinished(fileList);
}
