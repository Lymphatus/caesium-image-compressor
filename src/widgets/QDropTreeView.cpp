#include "QDropTreeView.h"

#include <QDragEnterEvent>
#include <QMimeData>
#include <QSettings>

#include <QFileInfo>
#include <services/Importer.h>
#include <utils/Utils.h>

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
            if (QFileInfo(absolutePath).isFile()) {
                fileList << url.toLocalFile();
            } else if (QFileInfo(absolutePath).isDir()) {
                QSettings settings;
                bool scanSubfolders = settings.value("preferences/general/import_subfolders", true).toBool();
                fileList.append(Importer::scanDirectory(absolutePath, scanSubfolders));
            }
        }
    }

    event->acceptProposedAction();
    emit dropFinished(fileList);
}
