#ifndef CIMAGE_H
#define CIMAGE_H

#include <QFileInfo>
#include <QString>

#include "../utils/Utils.h"

struct CCSParameters {
    bool keep_metadata;
    unsigned int jpeg_quality;
    unsigned int png_level;
    bool png_force_zopfli;
    unsigned int gif_quality;
    unsigned int webp_quality;
    bool optimize;
    unsigned int width;
    unsigned int height;
};

struct CCSResult {
    bool success;
    char* error_message;
};

extern "C" CCSResult c_compress(const char* i, const char* o, CCSParameters params);

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
    QString getCompressedFullPath() const;
    size_t getTotalPixels() const;
    QString getFormattedStatus() const;
    QString getDirectory() const;
    QString getCompressedDirectory() const;

private:
    CImageStatus status;
    QString fullPath;
    QString fileName;
    QString compressedFullPath;
    QString directory;
    QString compressedDirectory;
    QString additionalInfo;

private:
    size_t size;
    size_t compressedSize;

    int width;
    int height;
    int compressedWidth;
    int compressedHeight;

    void setCompressedInfo(QFileInfo fileInfo);
    void setFileDates(QFileInfo fileInfo, CompressionOptions compressionOptions, FileDates inputFileDates);
};

#endif // CIMAGE_H
