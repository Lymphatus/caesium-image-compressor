#ifndef UTILS_H
#define UTILS_H
#ifdef __APPLE__
#define _LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR
#endif

#include <QImage>
#include <QMap>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QDateTime>

enum class CImageStatus {
    UNCOMPRESSED,
    COMPRESSING,
    COMPRESSED,
    ERROR
};

enum CImageColumns {
    NAME = 0,
    SIZE = 1,
    RESOLUTION = 2,
    RATIO = 3
};

enum ResizeMode {
    NO_RESIZE = 0,
    DIMENSIONS = 1,
    PERCENTAGE = 2,
    SHORT_EDGE = 3,
    LONG_EDGE = 4
};

typedef struct CsLocale {
    QString locale;
    QString label;
} CsLocale;

typedef struct FileDatesOutputOption {
    bool keepCreation;
    bool keepLastModified;
    bool keepLastAccess;
} FileDatesOutputOption;

typedef struct CompressionOptions {
    QString outputPath;
    QString basePath;
    QString suffix;
    bool lossless;
    bool keepMetadata;
    bool keepStructure;
    bool resize;
    int fitTo;
    int width;
    int height;
    int size;
    bool doNotEnlarge;
    bool sameFolderAsInput;
    int jpeg_quality;
    int png_level;
    bool keepDates;
    FileDatesOutputOption datesMap;
} CompressionOptions;

typedef struct FileDates {
    QDateTime creation;
    QDateTime lastModified;
    QDateTime lastAccess;
} FileDates;

typedef struct CompressionSummary {
    unsigned int totalImages = 0;
    size_t totalUncompressedSize = 0;
    size_t totalCompressedSize = 0;
} CompressionSummary;

const int LANGUAGES_COUNT = 3;

const CsLocale LANGUAGES[LANGUAGES_COUNT] = {
    { QString("default"), QString("Default") },
    { QString("en_US"), QString("English (US)") },
    { QString("it_IT"), QString("Italiano") },
};

// Utilities
QString toHumanSize(size_t size);
QStringList scanDirectory(QString directory);
// cs_image_pars getCompressionParametersFromLevel(int level, bool lossless, bool keepMetadata);
QString getRootFolder(QMap<QString, int> folderMap);
QImage cResize(QImage image, int fitTo, int width, int height, int size, bool doNotEnlarge);
bool copyMetadata(const char* input, const char* output);
#endif // UTILS_H
