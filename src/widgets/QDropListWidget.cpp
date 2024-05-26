#include "QDropListWidget.h"

#include <QDragEnterEvent>
#include <QMimeData>
#include <QSettings>

#include <QFileInfo>
#include <services/Importer.h>
#include <utils/Utils.h>

QDropListWidget::QDropListWidget(QWidget* parent)
    : QListWidget(parent)
{
}

void QDropListWidget::dragEnterEvent(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void QDropListWidget::dragMoveEvent(QDragMoveEvent* event)
{
    event->accept();
}

void QDropListWidget::dropEvent(QDropEvent* event)
{
    emit dropFinished(event);
}
