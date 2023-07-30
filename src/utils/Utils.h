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

enum CompressionMode {
    QUALITY,
    SIZE
};

typedef struct CsCompressionMode {
    QString label;
    CompressionMode mode;
} CsCompressionMode;

enum MaxOutputSizeUnit {
    MAX_OUTPUT_BYTES,
    MAX_OUTPUT_KB,
    MAX_OUTPUT_MB,
    MAX_OUTPUT_PERCENTAGE,
};

typedef struct CsMaxOutputSizeUnit {
    QString label;
    MaxOutputSizeUnit unit;
} CsMaxOutputSizeUnit;

typedef struct MaxOutputSize {
    MaxOutputSizeUnit unit;
    size_t maxOutputSize;
} MaxOutputSize;

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
    bool moveToTrash;
    int jpeg_quality;
    int png_quality;
    int webp_quality;
    bool keepDates;
    FileDatesOutputOption datesMap;
    CompressionMode compressionMode;
    MaxOutputSize maxOutputSize;
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
    QString format;
} ImagePreview;

const int THEMES_COUNT = 2;
const CsTheme THEMES[THEMES_COUNT] = {
    { QString("Native"), QString("Native") },
    { QString("Fusion"), QString("Fusion") },
};

const int COMPRESSION_MODES_COUNT = 2;
const CsCompressionMode COMPRESSION_MODES[COMPRESSION_MODES_COUNT] = {
    { QIODevice::tr("Quality"), QUALITY },
    { QIODevice::tr("Size"), SIZE },
};

const int MAX_OUTPUT_UNITS_COUNT = 4;
const CsMaxOutputSizeUnit MAX_OUTPUT_UNITS[MAX_OUTPUT_UNITS_COUNT] = {
    { QIODevice::tr("bytes"), MAX_OUTPUT_BYTES },
    { QIODevice::tr("KB"), MAX_OUTPUT_KB },
    { QIODevice::tr("MB"), MAX_OUTPUT_MB },
    { QIODevice::tr("%"), MAX_OUTPUT_PERCENTAGE },
};

// Utilities
QString toHumanSize(double size);
QStringList scanDirectory(const QString& directory, bool subfolders);
QString getRootFolder(QList<QString> folderMap);
std::tuple<unsigned int, unsigned int> cResize(QSize originalSize, int fitTo, int width, int height, int size, bool doNotEnlarge);
void showFileInNativeFileManager(const QString& filePath, const QString& fallbackDirectory);
QJsonObject getSystemData();
QJsonObject getCompressionOptionsAsJSON();
QString getCompressionOptionsHash();
QString hashString(const QString& data, QCryptographicHash::Algorithm algorithm);
QStringList getOutputSupportedFormats();
#endif // UTILS_H
