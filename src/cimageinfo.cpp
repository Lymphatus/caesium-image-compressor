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

#include "cimageinfo.h"
#include "utils.h"

#include <QFileInfo>
#include <QImageReader>

CImageInfo::CImageInfo(QString path) {
    QFileInfo* fi = new QFileInfo(path);
    fullPath = path;
    baseName = fi->completeBaseName();
    size = fi->size();
    formattedSize = toHumanSize(size);
    QImageReader* ir = new QImageReader(path);
    QSize s = ir->size();
    width = s.width();
    height = s.height();
}

CImageInfo::CImageInfo() {

}

CImageInfo::~CImageInfo() {

}

QString CImageInfo::getFullPath() const {
    return fullPath;
}

void CImageInfo::setFullPath(const QString &value) {
    fullPath = value;
}

QString CImageInfo::getBaseName() const {
    return baseName;
}

void CImageInfo::setBaseName(const QString &value) {
    baseName = value;
}

int CImageInfo::getSize() const {
    return size;
}

void CImageInfo::setSize(int value) {
    size = value;
}

QString CImageInfo::getFormattedSize() const {
    return formattedSize;
}

void CImageInfo::setFormattedSize(const QString &value) {
    formattedSize = value;
}

bool CImageInfo::isEqual(QString path) {
    return (QString::compare(fullPath, path) == 0);
}

int CImageInfo::getHeight() const {
    return height;
}

void CImageInfo::setHeight(int value) {
    height = value;
}

int CImageInfo::getWidth() const {
    return width;
}

void CImageInfo::setWidth(int value) {
    width = value;
}

QString CImageInfo::getFormattedResolution() const {
    return QString::number(width) + "x" + QString::number(height);
}


