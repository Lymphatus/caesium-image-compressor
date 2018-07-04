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

#include "src/ctreewidgetitem.h"

/*
citem_status CTreeWidgetItem::getStatus() const {
    return status;
}

void CTreeWidgetItem::setStatus(const citem_status &value) {
    status = value;
    emit compressionStatusChanged(this, value);
}
*/

CTreeWidgetItem::CTreeWidgetItem(QTreeWidget *parent, QString path) : QTreeWidgetItem(parent)
{
    this->path = path;
}

CTreeWidgetItem::~CTreeWidgetItem()
{
    delete this->image;
}

QString CTreeWidgetItem::getPath() const
{
    return path;
}

void CTreeWidgetItem::setPath(const QString &value)
{
    path = value;
}

CImage *CTreeWidgetItem::getImage() const
{
    return image;
}

void CTreeWidgetItem::setImage(CImage *value)
{
    image = value;
    setColumnsText();
}

void CTreeWidgetItem::setColumnsText()
{
    this->setText(COLUMN_NAME, this->image->getFileName());
    this->setText(COLUMN_SIZE, this->image->getFormattedSize());
    this->setText(COLUMN_RESOLUTION, this->image->getResolution());
    this->setText(COLUMN_PATH, this->path);
}
