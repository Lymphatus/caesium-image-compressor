#ifndef UTILS_H
#define UTILS_H

#include <QCryptographicHash>
#include <QDateTime>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QJsonObject>
#include <QMap>
#include <QPixmap>
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

typedef struct CsTheme {
    QString theme;
    QString label;
} CsTheme;

typedef struct FileDatesOutputOption {
    bool keepCreation;
    bool keepLastModified;
    bool keepLastAccess;
} FileDatesOutputOption;

typedef struct CompressionOptions {
    QString outputPath;
    QString basePath;
    QString suffix;
    int format;
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
    bool skipIfBigger;
    int jpeg_quality;
    int png_quality;
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

typedef struct ImagePreview {
    QPixmap image;
    QFileInfo fileInfo;
    size_t originalSize;
    bool isOnFlyPreview;
} ImagePreview;

const int LANGUAGES_COUNT = 11;

const CsLocale LANGUAGES[LANGUAGES_COUNT] = {
    { QString("default"), QString("Default") },
    { QString("en_US"), QString("English (US)") },
    { QString("it_IT"), QString("Italiano") },
    { QString("fr_FR"), QString("Français") },
    { QString("ja_JP"), QString("日本語") },
    { QString("zh_CN"), QString("简体中文") },
    { QString("es_ES"), QString("Español") },
    { QString("ar_EG"), QString("العَرَبِيَّة") },
    { QString("de_DE"), QString("Deutsch") },
    { QString("ru_RU"), QString("Русский") },
    { QString("tr_TR"), QString("Türkçe") },
};

const int THEMES_COUNT = 3;
const CsTheme THEMES[THEMES_COUNT] = {
    { QString("Native"), QString("Native") },
    { QString("Fusion"), QString("Fusion (Dark)") },
    { QString("Fusion"), QString("Fusion (Light)") }
};

const QStringList OUTPUT_SUPPORTED_FORMATS = {
    QIODevice::tr("Same as input"),
    QIODevice::tr("JPG"),
    QIODevice::tr("PNG"),
    QIODevice::tr("WebP"),
};

// Utilities
QString toHumanSize(double size);
QStringList scanDirectory(QString directory, bool subfolders);
QString getRootFolder(QMap<QString, int> folderMap);
std::tuple<unsigned int, unsigned int> cResize(QSize originalSize, int fitTo, int width, int height, int size, bool doNotEnlarge);
void showFileInNativeFileManager(const QString& filePath, const QString& fallbackDirectory);
QJsonObject getSystemData();
QJsonObject getCompressionOptionsAsJSON();
QString getCompressionOptionsHash();
QString hashString(const QString& data, QCryptographicHash::Algorithm algorithm);
#endif // UTILS_H
