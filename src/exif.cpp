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

#include <exiv2/exiv2.hpp>
#include "exif.h"
#include "utils.h"

#include <QString>
#include <QStringList>
#include <QCoreApplication>
#include <QDebug>

#include <stdlib.h>
#include <assert.h>


Exiv2::ExifData getExifFromPath(char* filename) {
    qDebug() << "Trying to read EXIF for" << filename;
    try {
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename);
        assert(image.get() != 0);
        image->readMetadata();

        Exiv2::ExifData &exifData = image->exifData();
        if (exifData.empty()) {
            std::string error(filename);
            error += ": No Exif data found in the file";

            //TODO Translate
            //throw Exiv2::Error(1, error);
        }
        return exifData;
    } catch (Exiv2::Error& e) {
        Exiv2::ExifData exifData;
        qCritical() << "Caught Exiv2 exception '" << e.what();
        //TODO Translate
        return exifData;
    }


}

QString exifDataToString(Exiv2::ExifData exifData) {
    if (exifData.empty()) {
        //TODO Translate
        return QString("No EXIF found");
    }
    try {
        Exiv2::ExifData::const_iterator end = exifData.end();
        QString tmpExif, final;
        for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
            const char* tn = i->typeName();
            /*std::string str = i->key() << " ";
                      << "0x" << std::setw(4) << std::setfill('0') << std::right
                      << std::hex << i->tag() << " "
                      << std::setw(9) << std::setfill(' ') << std::left
                      << (tn ? tn : "Unknown") << " "
                      << std::dec << std::setw(3)
                      << std::setfill(' ') << std::right
                      << i->count() << "  "
                      << std::dec << i->value()
                      << "\n";*/
            tmpExif = "<span style='color:#1cb495;'>" + QString::fromStdString(i->key()).split(".").at(2) + "</span>&emsp;" +
                    //QString::number(i->tag()) + "\t" +
                    //(tn ? tn : "Unknown") + "\t" +
                    //QString::number(i->count()) + "\t" +
                    QString::fromStdString(i->value().toString()) + "<br />";
            final.append(tmpExif);
        }
        return final;
    } catch(Exiv2::Error& e) {
        std::cout << "Caught Exiv2 exception '" << e.what() << "'\n";
        //TODO Translate
        return QString("Error while reading EXIF");
    }
}

void writeSpecificExifTags(Exiv2::ExifData exifData, QString imagePath, QList<cexifs> exifs) {
    //If tags are empty, jus return back
    if (exifData.empty()) {
        return;
    }
    //Get output file path
    std::string path = imagePath.toStdString();

    Exiv2::ExifData newExifData;

    foreach (cexifs cex, exifs) {
        if (cex == EXIF_COPYRIGHT) {
            writeExif(exifData, &newExifData, "Exif.Image.Copyright");
        } else if (cex == EXIF_DATE) {
            writeExif(exifData, &newExifData, "Exif.Image.DateTime");
            writeExif(exifData, &newExifData, "Exif.Image.DateTimeOriginal");
            writeExif(exifData, &newExifData, "Exif.Photo.DateTimeOriginal");
            writeExif(exifData, &newExifData, "Exif.Photo.DateTimeDigitized");
            writeExif(exifData, &newExifData, "Exif.Photo.SubSecTime");
            writeExif(exifData, &newExifData, "Exif.Photo.SubSecTimeOriginal");
            writeExif(exifData, &newExifData, "Exif.Photo.SubSecTimeDigitized");
            writeExif(exifData, &newExifData, "Exif.GPSInfo.GPSDateStamp");
        } else if (cex == EXIF_COMMENTS) {
            writeExif(exifData, &newExifData, "Exif.Photo.UserComment");
            writeExif(exifData, &newExifData, "Exif.Image.ImageDescription");
            writeExif(exifData, &newExifData, "Exif.Image.XPComment");
        }
    }

    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(path);
    assert(image.get() != 0);

    image->setExifData(newExifData);
    image->writeMetadata();
}

void writeExif(Exiv2::ExifData exifData, Exiv2::ExifData* newExifData, std::string key_name) {
    //TODO Errors
    try {
        Exiv2::Value::AutoPtr v = Exiv2::Value::create(Exiv2::asciiString);
        Exiv2::ExifData::iterator pos;

        //Get the tag and rewrite it
        Exiv2::ExifKey key = Exiv2::ExifKey(key_name);
        pos = exifData.findKey(key);
        if (pos != exifData.end()) {
            v = pos->getValue();
            newExifData->add(key, v.get());
        } else {
            //Not found
        }
    }
    catch (Exiv2::AnyError& e) {
        qWarning() << "Caught Exiv2 exception: " + QString(e.what()) + "\n";
    }
}
