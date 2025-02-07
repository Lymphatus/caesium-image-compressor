#ifndef CIMAGE_H
#define CIMAGE_H

#include "../include/libcaesium.h"
#include "../utils/Utils.h"

class CImage {
    const QList<QByteArray> supportedFormats = { "png", "jpg", "jpeg", "webp", "tiff" };

public:
    explicit CImage(const QString& path);

    friend bool operator==(const CImage& c1, const CImage& c2);
    friend bool operator!=(const CImage& c1, const CImage& c2);

    QString getFormattedSize() const;
    QString getRichFormattedSize() const;
    QString getResolution() const;
    QString getRichResolution() const;
    QString getFileName() const;
    QString getFullPath() const;
    size_t getOriginalSize() const;
    size_t getCompressedSize() const;
    CImageStatus getStatus() const;
    void setStatus(const CImageStatus& value);
    double getRatio() const;
    QString getFormattedSavedRatio() const;
    QString getRichFormattedSavedRatio() const;
    bool compress(const CompressionOptions& compressionOptions);
    bool preview(const CompressionOptions& compressionOptions) const;
    QString getCompressedFullPath() const;
    QString getTemporaryPreviewFullPath() const;
    QString getPreviewFullPath() const;
    size_t getTotalPixels() const;
    QString getFormattedStatus() const;
    QString getDirectory() const;
    QString getCompressedDirectory() const;
    QString getHashedFullPath() const;
    QString getFormat() const;
    CCSParameters getCSParameters(const CompressionOptions& compressionOptions) const;

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

    size_t size;
    size_t compressedSize;

    int width;
    int height;
    int compressedWidth;
    int compressedHeight;

    void setCompressedInfo(const QFileInfo& fileInfo);
    static void setFileDates(const QFileInfo& fileInfo, FileDatesOutputOption datesMap, const FileDates& inputFileDates);
    static size_t getMaxOutputSizeInBytes(MaxOutputSize maxOutputSize, size_t originalSize);
};

#endif // CIMAGE_H
