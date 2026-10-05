#include "QDropTreeView.h"

#include <QDragEnterEvent>
#include <QMimeData>

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
            if (url.isLocalFile()) {
                fileList << url.toLocalFile();
            }
        }
    }

    event->acceptProposedAction();
    emit dropFinished(fileList);
}
