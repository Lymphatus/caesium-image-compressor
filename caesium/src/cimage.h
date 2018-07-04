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
#ifndef CIMAGE_H
#define CIMAGE_H

#include <QString>

class CImage
{
public:
    CImage(QString path);
    CImage();

    QString getFormattedSize();
    QString getFormattedSize(size_t size);

    QString getResolution();
    QString getResolution(int w, int h);

    QString getFileName() const;

private:
    QString fullPath;
    QString fileName;

    size_t size;
    QString formattedSize;

    int width;
    int height;
    QString resolution;

};

#endif // CIMAGE_H
