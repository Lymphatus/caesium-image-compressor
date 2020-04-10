#ifndef CIMAGE_H
#define CIMAGE_H

#include <QFileInfo>
#include <QString>

#include <src/utils/Utils.h>

class CImage
{

public:
    explicit CImage(const QString& path);
    //TODO Destructor

    friend bool operator== (const CImage &c1, const CImage &c2);
    friend bool operator!= (const CImage &c1, const CImage &c2);

    QString getFormattedSize();

    QString getResolution();

    QString getFileName() const;

    QString getFullPath() const;

    CImageStatus getStatus() const;
    void setStatus(const CImageStatus &value);

    double getRatio() const;
    QString getFormattedSavedRatio();

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
    QString formattedSize;
    QString compressedFormattedSize;

    int width;
    int height;
    int compressedWidth;
    int compressedHeight;

};

#endif // CIMAGE_H
