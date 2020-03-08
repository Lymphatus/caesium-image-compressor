#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QSize>
#include <QStringList>

#include <src/vendor/caesium.h>

enum class CImageStatus {
    UNCOMPRESSED,
    COMPRESSING,
    COMPRESSED,
    ERROR
};

enum CImageColumns
{
    NAME = 0,
    SIZE = 1,
    RESOLUTION = 2,
    RATIO = 3
};

//Utilities
QString toHumanSize(size_t size);
QSize getScaledSizeWithRatio(QSize size, int square);
QStringList scanDirectory(QString directory);
cs_image_pars getCompressionParametersFromLevel(int level, bool lossless, bool keepMetadata);

#endif // UTILS_H
