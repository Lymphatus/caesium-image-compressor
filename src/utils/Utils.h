#ifndef UTILS_H
#define UTILS_H

#include <QCryptographicHash>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonObject>
#include <QPixmap>
#include <QSize>
#include <QString>

enum class CImageStatus {
    UNCOMPRESSED,
    COMPRESSING,
    COMPRESSED,
    ERROR,
    WARNING
};

constexpr int CIMAGE_COLUMNS_SIZE = 5;

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
    LONG_EDGE = 4,
    FIXED_WIDTH = 5,
    FIXED_HEIGHT = 6
};

typedef struct CsLocale {
    QString locale;
    QString label;
} CsLocale;

enum CompressionMode {
    QUALITY = 0,
    SIZE = 1
};

enum MaxOutputSizeUnit {
    MAX_OUTPUT_BYTES = 0,
    MAX_OUTPUT_KB = 1,
    MAX_OUTPUT_MB = 2,
    MAX_OUTPUT_PERCENTAGE = 3,
};

enum class PostCompressionAction {
    NO_ACTION,
    CLOSE_APPLICATION,
    SLEEP,
    SHUTDOWN,
    OPEN_FOLDER
};

enum JPEGChromaSubsampling {
    CHROMA_AUTO = 0,
    CHROMA_444 = 444,
    CHROMA_422 = 422,
    CHROMA_420 = 420,
    CHROMA_411 = 411,
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

typedef struct ChromaSubsampling {
    QString label;
    int value;
} ChromaSubsampling;

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
    bool moveOriginalFile;
    int moveOriginalFileDestination;
    int jpegQuality;
    int jpegChromaSubsampling;
    int jpegProgressive;
    int pngQuality;
    int pngOptimizationLevel;
    int webpQuality;
    int tiffMethod;
    int tiffDeflateLevel;
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

constexpr int THEMES_COUNT = 2;
const CsTheme THEMES[THEMES_COUNT] = {
    { QString("Native"), QString("Native") },
    { QString("Fusion"), QString("Fusion") },
};

// Utilities
QString toHumanSize(double size);
std::tuple<unsigned int, unsigned int> cResize(const QImageReader* reader, const CompressionOptions& compressionOptions);
QSize getSizeWithMetadata(const QImageReader* reader);
bool isRotatedByMetadata(const QImageReader* reader);
void showFileInNativeFileManager(const QString& filePath, const QString& fallbackDirectory);
void showDirectoryInNativeFileManager(const QString& dirPath);
QJsonObject getSystemData();
QJsonObject getCompressionOptionsAsJSON();
QString getCompressionOptionsHash();
QString hashString(const QString& data, QCryptographicHash::Algorithm algorithm);
QStringList getOutputSupportedFormats();
QMap<int, QString> getChromaSubsamplingOptions();
#endif // UTILS_H
