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

#include "utils.h"
#include "math.h"
#include <stdlib.h>

#include <QIODevice>
#include <QDate>
#include <QTreeWidgetItem>
#include <QDirIterator>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QDebug>

QString clfFilter = "Caesium List File (*.clf)";
QStringList inputFilterList = QStringList() << "*.jpg" << "*.jpeg" << "*.png";
QString versionString = "2.0.0-beta1";
int versionNumber = 00;
int buildNumber = 20160630;
QString updateVersionTag = "";
long originalsSize = 0;
long compressedSize = 0;
int compressedFiles = 0;
c_parameters params;
QStringList osAndExtension = QStringList() <<
        #ifdef _WIN32
            "win" << ".exe";
        #elif __APPLE__
            "osx" << ".dmg";
        #else
            "linux" << ".tar.gz";
        #endif
QTemporaryDir tempDir;
QElapsedTimer timer;
QString lastCListPath = "";
QList<QLocale> locales;
QString logPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
        "/" +
        QDate::currentDate().toString("caesium_dd_MM_yyyy.log");

QString toHumanSize(long size) {
    //Check if size is 0 to avoid crashes
    if (size == 0) {
        return "0 bytes";
    }

    QStringList unit;
    unit << "Bytes" << "Kb" << "Mb" << "Gb" << "Tb";
    //Index of the array containing the correct unit
    double order = floor(log2(labs(size)) / 10);

    //We should never handle files over 1k Tb, but...
    if (order > 4) {
        qWarning() << "Woah, huge collection!";
        order = 4;
    }

    return QString::number(size / (pow(1024, order)), 'f', 2) + unit[(int)order];
}

double humanToDouble(QString human_size) {
    QStringList splitted = human_size.split(" ");
    if (splitted.at(1) == "Kb") {
        return QString(splitted.at(0)).toDouble() * 1024;
    } else if (splitted.at(1) == "Mb") {
        return QString(splitted.at(0)).toDouble() * 1048576;
    } else {
        return QString(splitted.at(0)).toDouble();
    }
}

QString getRatio(qint64 original, qint64 compressed) {
    return QString::number(((float) ((original - compressed) * 100) / (float) original), 'f', 1) + "%";
}

char* QStringToChar(QString s) {
    char* c_str = (char*) malloc((s.length() + 1) * sizeof(char));
    QByteArray bArray = s.toLocal8Bit();
    strcpy(c_str, bArray.data());
    return c_str;
}

QSize getScaledSizeWithRatio(QSize size, int square) {
    int w = size.width();
    int h = size.height();

    double ratio = 0.0;

    //Check the biggest between the two and scale on that dimension
    if (w >= h) {
        ratio = w / (double) square;
    } else {
        ratio = h / (double) square;
    }

    return QSize((int) round(w / ratio), (int) h / ratio);
}

double ratioToDouble(QString ratio) {
    ratio = ratio.split("%").at(0);
    return ratio.toDouble();
}

bool isJPEG(char* path) {
    FILE* fp;
    unsigned char* type_buffer = (unsigned char*) malloc(2);

    fp = fopen(path, "r");

    if (fp == NULL) {
        qWarning() << "Cannot open" <<  path << "for type detection. Skipping";
        return false;
    }

    if (fread(type_buffer, 1, 2, fp) < 2) {
        qWarning() << "Cannot read" <<  path << "type. Skipping";
        return false;
    }

    fclose(fp);

    if (((int) type_buffer[0] == 0xFF) && ((int) type_buffer[1] == 0xD8)) {
        free(type_buffer);
        return true;
    } else {
        fprintf(stderr, "Unsupported file type. Skipping.\n");
        return false;
    }
}

enum image_type detect_image_type(char* path) {
    //Open the file
    FILE* fp;
    unsigned char* type_buffer = (unsigned char*) malloc(2);

    fp = fopen(path, "r");

    if (fp == NULL) {
        fprintf(stderr, "Cannot open input file for type detection. Skipping.\n");
        return UNKN;
    }
    //Read enough bytes
    if (fread(type_buffer, 1, 2, fp) < 2) {
        fprintf(stderr, "Cannot read file type. Skipping.\n");
        free(type_buffer);
        return UNKN;
    }
    //We don't need it anymore
    fclose(fp);

    //Check the bytes against the JPEG and PNG specs
    if (((int) type_buffer[0] == 0xFF) && ((int) type_buffer[1] == 0xD8)) {
        free(type_buffer);
        return JPEG;
    } else if (((int) type_buffer[0] == 0x89) && ((int) type_buffer[1] == 0x50)) {
        free(type_buffer);
        return PNG;
    } else {
        free(type_buffer);
        return UNKN;
    }
}

QString msToFormattedString(qint64 ms) {
    if (ms < 1000) {
        return QString::number(ms) + " ms";
    } else if (ms >= 1000 && ms < 60000) {
        return QString::number(((double) ms) / 1000, 'f', 1) + "s";
    } else {
        return QString::number(ms / 60000) + ":"
                + (ms < 70000 ? "0" : "") +
                QString::number(ms / 1000 % 60) +
                QT_TRANSLATE_NOOP("Utils"," minutes");
    }
}

bool haveSameRootFolder(QList<QTreeWidgetItem *> items) {
    QDir root = QFileInfo(items.at(0)->text(COLUMN_PATH)).dir();
    for (int i = 1; i < items.length(); i++) {
        if (QString::compare(QFileInfo(items.at(i)->text(COLUMN_PATH)).dir().absolutePath(),
                             root.absolutePath(),
                             Qt::CaseSensitive) != 0) {
            return false;
        }
    }
    return true;
}

QString toCapitalCase(const QString str) {
    if (str.size() < 1) {
        return "";
    }

    QStringList tokens = str.split(" ");
    QList<QString>::iterator tokItr = tokens.begin();

    for (tokItr = tokens.begin(); tokItr != tokens.end(); ++tokItr) {
        (*tokItr) = (*tokItr).at(0).toUpper() + (*tokItr).mid(1);
    }

    return tokens.join(" ");
}

void loadLocales() {
    locales.insert(0, QLocale::system());
    QString translationPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    //Iterate trough files and get the language string
    QDirIterator it(translationPath, QStringList() << "*.qm");
    while (it.hasNext()) {
        it.next();
        locales.append(QLocale(it.fileInfo().baseName().replace("caesium_", "")));
    }
    //qInfo() << "Found locales" << locales;
}

