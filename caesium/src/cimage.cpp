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

#include "cimage.h"
#include "utils.h"

#include <QFileInfo>
#include <QImageReader>

#include <QDebug>

CImage::CImage(QString path) {
    QFileInfo fi = QFileInfo(path);
    fullPath = path;
    baseName = fi.completeBaseName();
    size = fi.size();
    formattedSize = toHumanSize(size);
    ir = new QImageReader(path);
    QSize s = ir->size();
    width = s.width();
    height = s.height();

    type = typeFormatToEnum(ir->format());
}

CImage::CImage() {

}

CImage::~CImage() {

}

QString CImage::getFullPath() const {
    return fullPath;
}

void CImage::setFullPath(const QString &value) {
    fullPath = value;
}

QString CImage::getBaseName() const {
    return baseName;
}

void CImage::setBaseName(const QString &value) {
    baseName = value;
}

int CImage::getSize() const {
    return size;
}

void CImage::setSize(int value) {
    size = value;
}

QString CImage::getFormattedSize() const {
    return formattedSize;
}

void CImage::setFormattedSize(const QString &value) {
    formattedSize = value;
}

bool CImage::isEqual(QString path) {
    return (QString::compare(fullPath, path) == 0);
}

int CImage::getHeight() const {
    return height;
}

void CImage::setHeight(int value) {
    height = value;
}

int CImage::getWidth() const {
    return width;
}

void CImage::setWidth(int value) {
    width = value;
}

QString CImage::getFormattedResolution() const {
    return QString::number(width) + "x" + QString::number(height);
}

image_type CImage::getType() const
{
    return type;
}

void CImage::setType(const image_type &value)
{
    type = value;
}

QString CImage::printPNGParams()
{
//    QString final = "[";
//    final.append(QObject::tr("Iterations: ") + QString::number(pparams.getIterations()) + ", ");
//    final.append(QObject::tr("Iterations large: ") + QString::number(pparams.getIterationsLarge()) + ", ");
//    final.append(QObject::tr("8 bit: "));
//    final.append(pparams.getLossy8Bit() != 0 ? QObject::tr("yes") : QObject::tr("no"));
//    final.append("]");

//    return final;
}

QString CImage::printJPEGParams()
{
//    QString final = "[";
//    if (jparams.quality <= 0) {
//        final.append(QObject::tr("Lossless") + ", ");
//    } else {
//        final.append(QObject::tr("Quality: ") + QString::number(jparams.quality) + ", ");
//    }
//    final.append(QObject::tr("Exif: "));
//    if (!jparams.exif) {
//        final.append(QObject::tr("no") + ", ");
//    } else if (jparams.importantExifs.length() == 3) {
//        final.append(QObject::tr("all") + ", ");
//    } else {
//        if (jparams.importantExifs.contains(EXIF_COPYRIGHT)) {
//           final.append(QObject::tr("copyright") + ", ");
//        }
//        if (jparams.importantExifs.contains(EXIF_DATE)) {
//           final.append(QObject::tr("date") + ", ");
//        }
//        if (jparams.importantExifs.contains(EXIF_COMMENTS)) {
//           final.append(QObject::tr("comments") + ", ");
//        }
//    }
//    final.append(QObject::tr("Progressive: "));
//    final.append(jparams.quality == 0 ? QObject::tr("yes") : QObject::tr("no"));
//    final.append("]");
//    return final;
}
