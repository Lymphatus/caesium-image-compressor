#include "qdroptreewidget.h"
#include "caesium.h"

#include <QStringList>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMetaObject>

QDropTreeWidget::QDropTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{

}

void QDropTreeWidget::dragEnterEvent(QDragEnterEvent *event) {
    event->acceptProposedAction();
}

void QDropTreeWidget::dragMoveEvent(QDragMoveEvent *event) {
    event->accept();
}

void QDropTreeWidget::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    QList<QUrl> urlList = mimeData->urls();
    QStringList fileList;
    if (mimeData->hasFormat("text/uri-list")) {
        foreach (QUrl url, urlList) {
            fileList << url.toLocalFile();
        }
    }
    event->acceptProposedAction();
    emit dropFinished(fileList);
}
