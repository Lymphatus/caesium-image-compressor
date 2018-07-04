/*
 *
 * This file is part of Caesium Image Compressor.
 *
 * Caesium Image Compressor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Caesium Image Compressor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Caesium Image Compressor.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "cimage.h"
#include "src/utils.h"

#include <QFileInfo>
#include <QImageReader>


CImage::CImage(QString path)
{
    QFileInfo fileInfo = QFileInfo(path);
    QImageReader *imageReader = new QImageReader(path);
    QSize imageSize = imageReader->size();

    this->fullPath = path;
    this->fileName = fileInfo.completeBaseName();
    this->size = fileInfo.size();

    this->width = imageSize.width();
    this->height = imageSize.height();

    delete imageReader;
}

CImage::CImage() {}

QString CImage::getFormattedSize()
{
    return toHumanSize(this->size);
}

QString CImage::getFormattedSize(size_t size)
{
    return toHumanSize(size);
}

QString CImage::getResolution()
{
    return QString::number(this->width) + "x" + QString::number(this->height);
}

QString CImage::getResolution(int w, int h)
{
    return QString::number(w) + "x" + QString::number(h);
}

QString CImage::getFileName() const
{
    return fileName;
}
