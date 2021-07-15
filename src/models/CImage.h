#ifndef CIMAGE_H
#define CIMAGE_H

#include <QFileInfo>
#include <QString>

#include "../utils/Utils.h"

struct C_CSParameters {
    bool keep_metadata;
    unsigned int jpeg_quality;
    unsigned int png_level;
    bool optimize;
};
extern "C" bool c_compress(const char *i, const char *o, C_CSParameters params);

class CImage
{
    const QList<QByteArray> supportedFormats = {"png", "jpg", "jpeg"};

public:
    explicit CImage(const QString& path);
    //TODO Destructor

    friend bool operator== (const CImage &c1, const CImage &c2);
    friend bool operator!= (const CImage &c1, const CImage &c2);

    QString getFormattedSize();
    QString getRichFormattedSize();

    QString getResolution();
    QString getRichResolution();

    QString getFileName() const;

    QString getFullPath() const;

    size_t getOriginalSize() const;
    size_t getCompressedSize() const;

    CImageStatus getStatus() const;
    void setStatus(const CImageStatus &value);

    double getRatio() const;
    QString getFormattedSavedRatio();
    QString getRichFormattedSavedRatio();

    bool compress(CompressionOptions compressionOptions);
    void setCompressedInfo(QFileInfo fileInfo);

    QString getCompressedFullPath() const;

private:
    CImageStatus status;
    QString fullPath;
    QString fileName;
    QString compressedFullPath;

    size_t size;
    size_t compressedSize;

    int width;
    int height;
    int compressedWidth;
    int compressedHeight;

};

#endif // CIMAGE_H
