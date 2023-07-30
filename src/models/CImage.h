#ifndef CIMAGE_H
#define CIMAGE_H

#include <QFileInfo>
#include <QString>

#include "../utils/Utils.h"

typedef struct CCSParameters {
    bool keep_metadata;
    unsigned int jpeg_quality;
    unsigned int jpeg_chroma_subsampling;
    unsigned int png_quality;
    bool png_force_zopfli;
    unsigned int gif_quality;
    unsigned int webp_quality;
    bool optimize;
    int width;
    int height;
} CCSParameters;

struct CCSResult {
    bool success;
    char* error_message;
};

extern "C" CCSResult c_compress(const char* i, const char* o, struct CCSParameters params);
extern "C" CCSResult c_compress_to_size(const char* i, const char* o, struct CCSParameters params, size_t maxSize);

class CImage {
    const QList<QByteArray> supportedFormats = { "png", "jpg", "jpeg", "webp" };

public:
    explicit CImage(const QString& path);

    friend bool operator==(const CImage& c1, const CImage& c2);
    friend bool operator!=(const CImage& c1, const CImage& c2);

    QString getFormattedSize();
    QString getRichFormattedSize();
    QString getResolution();
    QString getRichResolution();
    QString getFileName() const;
    QString getFullPath() const;
    size_t getOriginalSize() const;
    size_t getCompressedSize() const;
    CImageStatus getStatus() const;
    void setStatus(const CImageStatus& value);
    double getRatio() const;
    QString getFormattedSavedRatio();
    QString getRichFormattedSavedRatio();
    bool compress(const CompressionOptions& compressionOptions);
    bool preview(const CompressionOptions& compressionOptions);
    QString getCompressedFullPath() const;
    QString getTemporaryPreviewFullPath() const;
    QString getPreviewFullPath() const;
    size_t getTotalPixels() const;
    QString getFormattedStatus() const;
    QString getDirectory() const;
    QString getCompressedDirectory() const;
    QString getHashedFullPath() const;
    QString getFormat() const;
    CCSParameters getCSParameters(const CompressionOptions& compressionOptions);

private:
    CImageStatus status;
    QString fullPath;
    QString fileName;
    QString compressedFullPath;
    QString directory;
    QString compressedDirectory;
    QString additionalInfo;
    QString hashedFullPath;
    QString extension;
    QString format;

private:
    size_t size;
    size_t compressedSize;

    int width;
    int height;
    int compressedWidth;
    int compressedHeight;

    void setCompressedInfo(QFileInfo fileInfo);
    void setFileDates(QFileInfo fileInfo, FileDatesOutputOption datesMap, FileDates inputFileDates);
    QSize getSizeWithMetadata(QImageReader* reader);
    size_t getMaxOutputSizeInBytes(MaxOutputSize maxOutputSize, size_t originalSize);
};

#endif // CIMAGE_H
