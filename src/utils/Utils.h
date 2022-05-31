#ifndef UTILS_H
#define UTILS_H

#include <QDateTime>
#include <QImage>
#include <QImageReader>
#include <QJsonObject>
#include <QMap>
#include <QSize>
#include <QString>
#include <QStringList>

enum class CImageStatus {
    UNCOMPRESSED,
    COMPRESSING,
    COMPRESSED,
    ERROR,
    WARNING
};

const int CIMAGE_COLUMNS_SIZE = 5;

enum CImageColumns {
    NAME_COLUMN = 0,
    SIZE_COLUMN = 1,
    RESOLUTION_COLUMN = 2,
    RATIO_COLUMN = 3,
    INFO_COLUMN = 4
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
    int webp_quality;
    bool keepDates;
    FileDatesOutputOption datesMap;
} CompressionOptions;

typedef struct FileDates {
    QDateTime creation;
    QDateTime lastModified;
    QDateTime lastAccess;
} FileDates;

typedef struct CompressionSummary {
    unsigned long int totalImages = 0;
    double totalUncompressedSize = 0;
    double totalCompressedSize = 0;
    qint64 elapsedTime = 0;
} CompressionSummary;

const int LANGUAGES_COUNT = 8;

const CsLocale LANGUAGES[LANGUAGES_COUNT] = {
    { QString("default"), QString("Default") },
    { QString("en_US"), QString("English (US)") },
    { QString("it_IT"), QString("Italiano") },
    { QString("fr_FR"), QString("Français") },
    { QString("ja_JP"), QString("日本語") },
    { QString("zh_CN"), QString("简体中文") },
    { QString("es_ES"), QString("Español") },
    { QString("ar_EG"), QString("العامية المصرية") },
};

const unsigned short WIN32_LANGUAGES[LANGUAGES_COUNT] = {
    0x0409, 0x0409, 0x0410, 0x040C, 0x0411, 0x0804, 0x0c0A, 0x0C01
};

const int THEMES_COUNT = 2;
const QString THEMES[THEMES_COUNT] = {
    QString("Native"),
    QString("Fusion")
};

// Utilities
QString toHumanSize(double size);
QStringList scanDirectory(QString directory, bool subfolders);
QString getRootFolder(QMap<QString, int> folderMap);
std::tuple<unsigned int, unsigned int> cResize(QSize originalSize, int fitTo, int width, int height, int size, bool doNotEnlarge);
void showFileInNativeFileManager(const QString& filePath, const QString& fallbackDirectory);
QJsonObject getSystemData();
QJsonObject getCompressionOptionsAsJSON();
#endif // UTILS_H
