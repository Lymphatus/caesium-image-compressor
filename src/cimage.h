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
#include <QObject>

#include "src/utils.h"

typedef struct png_params {
    int iterations = 15;
    int iterationsLarge = 5;
    int blockSplitStrategy = 4;
    int lossy8Bit = 1;
    int transparent = 1;
    int autoFilterStrategy = 1;

public:
    int getIterations() const;
    void setIterations(int value);
    int getIterationsLarge() const;
    void setIterationsLarge(int value);
    int getLossy8Bit() const;
    void setLossy8Bit(int value);
    int getBlockSplitStrategy() const;
    int getTransparent() const;
    int getAutoFilterStrategy() const;
} png_params;

typedef struct jpeg_params {
    int quality = 65;
    int color_space = TJCS_RGB;
    int dct_method = TJFLAG_FASTDCT;
    bool exif = true;
    QList<cexifs> importantExifs = {};
    enum TJSAMP subsample;
    bool progressive = true;

public:
    bool getProgressive() const;
    void setProgressive(bool value);
    int getQuality() const;
    void setQuality(int value);
    bool getExif() const;
    void setExif(bool value);
    QList<cexifs> getImportantExifs() const;
    void setImportantExifs(const QList<cexifs> &value);
    void setDct_method(int value);
    void setSubsample(const TJSAMP &value);
    void setColor_space(int value);
    int getColor_space() const;
    int getDct_method() const;
    TJSAMP getSubsample() const;
} jpeg_params;

class CImage
{
    
public:
    explicit CImage(QString path);
    explicit CImage();
    virtual ~CImage();

    jpeg_params jparams;
    png_params pparams;

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

    image_type getType() const;
    void setType(const image_type &value);

    QString printPNGParams();
    QString printJPEGParams();

    void setParameters();

private:
    //Base params
    QString fullPath;
    QString baseName;
    int size;
    QString formattedSize;
    int width;
    int height;

    image_type type;
};

#endif // CIMAGEINFO_H
