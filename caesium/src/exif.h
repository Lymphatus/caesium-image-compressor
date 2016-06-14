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

#ifndef EXIF_H
#define EXIF_H

#include "utils.h"

#include <stdlib.h>
#include <QString>
#include <exiv2/exiv2.hpp>

Exiv2::ExifData getExifFromPath(char* filename);
QString exifDataToString(Exiv2::ExifData exifData);
void writeSpecificExifTags(Exiv2::ExifData exifData, QString imagePath, QList<cexifs> exifs);
void writeExif(Exiv2::ExifData exifData, Exiv2::ExifData* newExifData, std::string key_name);

#endif // EXIF_H

