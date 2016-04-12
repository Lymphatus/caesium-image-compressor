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

#define MAX_COLUMNS 7

typedef struct cclt_compress_parameters {
    int quality;
    int width;
    int height;
    char* output_folder;
    int color_space;
    int dct_method;
    int exif_copy;
    int lossless;
    char** input_files;
    int input_files_count;
    int subsample;
    int recursive;
    int structure;
} cclt_compress_parameters;

enum cexifs {
    EXIF_COPYRIGHT,
    EXIF_DATE,
    EXIF_COMMENTS
};

enum list_columns {
    COLUMN_NAME = 0,
    COLUMN_ORIGINAL_SIZE = 1,
    COLUMN_NEW_SIZE = 2,
    COLUMN_ORIGINAL_RESOLUTION = 3,
    COLUMN_NEW_RESOLUTION = 4,
    COLUMN_SAVED = 5,
    COLUMN_PATH = 6
};

typedef struct var {
    int exif;
    QList<cexifs> importantExifs;
    int progressive;
    bool overwrite;
    int outMethodIndex;
    QString outMethodString;
} cparams;

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
extern cparams params; //Important parameters
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

#endif // UTILS_H
