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
#include <QFutureWatcher>

#include <caesium.h>

#define MAX_COLUMNS 8

enum cexifs {
    EXIF_COPYRIGHT,
    EXIF_DATE,
    EXIF_COMMENTS
};

enum list_columns {
    COLUMN_STATUS = 0,
    COLUMN_NAME = 1,
    COLUMN_ORIGINAL_SIZE = 2,
    COLUMN_NEW_SIZE = 3,
    COLUMN_ORIGINAL_RESOLUTION = 4,
    COLUMN_NEW_RESOLUTION = 5,
    COLUMN_SAVED = 6,
    COLUMN_PATH = 7
};

typedef struct c_parameters {
    bool overwrite;
    int outMethodIndex;
    QString outMethodString;
} c_parameters;

enum citem_status {
    NEW = 0,
    COMPRESSED_OK = 10,
    COMPRESSED_ERROR = 11,
    PREVIEWED = 1,
    COMPRESSING = 2
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
extern QString lastCListPath; //Path of the last list saved
extern QList<QLocale> locales;
extern QString logPath; //Log file path

QString toHumanSize(long);
double humanToDouble(QString);
QString getRatio(qint64, qint64);
char* QStringToChar(QString s);
QSize getScaledSizeWithRatio(QSize size, int square); //Image preview resize
double ratioToDouble(QString ratio);
QString msToFormattedString(qint64);
bool haveSameRootFolder(QList<QTreeWidgetItem *> items);
QString toCapitalCase(const QString);
void loadLocales();
c_parameters initialize_compression_parameters();
image_type typeFormatToEnum(QByteArray format);
#endif // UTILS_H
