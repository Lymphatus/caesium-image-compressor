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

