/**
 *
 * This file is part of Caesium.
 *
 * Caesium - Caesium is an image compression software aimed at helping photographers,
 * bloggers, webmasters, businesses or casual users at storing, sending and sharing digital pictures.
 *
 * Copyright (C) 2016 - Matteo Paonessa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>
 *
 */

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
