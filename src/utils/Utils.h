#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QSize>
#include <QStringList>
#include <QMap>

#include "../vendor/caesium.h"

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

typedef struct CompressionOptions
{
    QString outputPath;
    QString basePath;
    QString suffix;
    int compressionLevel;
    bool lossless;
    bool keepMetadata;
    bool keepStructure;

} CompressionOptions;

//Utilities
QString toHumanSize(size_t size);
QStringList scanDirectory(QString directory);
cs_image_pars getCompressionParametersFromLevel(int level, bool lossless, bool keepMetadata);
QString getRootFolder(QMap<QString, int> folderMap);

#endif // UTILS_H
