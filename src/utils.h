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

#ifndef UTILS_H
#define UTILS_H

#include <QList>
#include <QStringList>
#include <QSize>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QTreeWidgetItem>

#include <jpeglib.h>
#include <turbojpeg.h>

#define MAX_COLUMNS 7

enum cexifs {
    EXIF_COPYRIGHT,
    EXIF_DATE,
    EXIF_COMMENTS
};

typedef struct c_jpeg_parameters {
    int quality;
    int width;
    int height;
    int color_space;
    int dct_method;
    bool exif;
    QList<cexifs> importantExifs;
    bool lossless;
    enum TJSAMP subsample;
    bool progressive;
} cclt_jpeg_parameters;

typedef struct c_png_parameters {
    int iterations;
    int iterations_large;
    int block_split_strategy;
    int lossy_8;
    int transparent;
    int auto_filter_strategy;
} cclt_png_parameters;

typedef struct c_parameters {
    c_jpeg_parameters jpeg;
    c_png_parameters png;

    int new_width;
    int new_height;

    bool overwrite;
    int outMethodIndex;
    QString outMethodString;
} c_parameters;

enum list_columns {
    COLUMN_NAME = 0,
    COLUMN_ORIGINAL_SIZE = 1,
    COLUMN_NEW_SIZE = 2,
    COLUMN_ORIGINAL_RESOLUTION = 3,
    COLUMN_NEW_RESOLUTION = 4,
    COLUMN_SAVED = 5,
    COLUMN_PATH = 6
};

enum png_levels {
    VERY_LOW = 0,
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    VERY_HIGH = 4
};

enum image_type {
    JPEG,
    PNG,
    UNKN,
};

extern QString clfFilter;
extern QStringList inputFilterList;
extern QString versionString;
extern int versionNumber;
extern QString updateVersionTag;
extern int buildNumber;
extern long originalsSize, compressedSize; //Before and after bytes count
extern int compressedFiles; //Compressed files count
extern c_parameters params; //Important parameters
extern QStringList osAndExtension;
extern QTemporaryDir tempDir;
extern QElapsedTimer timer;
extern QString lastCListPath; //Path of the last list saved
extern QList<QLocale> locales;
extern QString logPath; //Log file path

QString toHumanSize(long);
double humanToDouble(QString);
QString getRatio(qint64, qint64);
char* QStringToChar(QString s);
QSize getScaledSizeWithRatio(QSize size, int square); //Image preview resize
double ratioToDouble(QString ratio);
bool isJPEG(char* path);
QString msToFormattedString(qint64);
bool haveSameRootFolder(QList<QTreeWidgetItem *> items);
QString toCapitalCase(const QString);
void loadLocales();
enum image_type detect_image_type(char* path);
c_parameters initialize_compression_parameters();

#endif // UTILS_H
