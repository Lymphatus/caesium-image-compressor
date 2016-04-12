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

#ifndef CIMAGEINFO_H
#define CIMAGEINFO_H

#include <QString>

class CImageInfo
{
public:
    CImageInfo(QString path);
    CImageInfo();
    virtual ~CImageInfo();

    QString getFullPath() const;
    void setFullPath(const QString &value);

    QString getBaseName() const;
    void setBaseName(const QString &value);

    int getSize() const;
    void setSize(int value);

    QString getFormattedSize() const;
    void setFormattedSize(const QString &value);

    bool isEqual(QString path);

    int getWidth() const;
    void setWidth(int value);

    int getHeight() const;
    void setHeight(int value);

    QString getFormattedResolution() const;

private:
    QString fullPath;
    QString baseName;
    int size;
    QString formattedSize;
    int width;
    int height;
};

#endif // CIMAGEINFO_H
