#include "CImage.h"
#include "src/utils/Utils.h"

#include <src/vendor/caesium.h>

#include <QFileInfo>
#include <QImageReader>
#include <cmath>

#include <QDebug>
#include <QDir>
#include <QSettings>


CImage::CImage(const QString& path)
{
    QFileInfo fileInfo = QFileInfo(path);
    auto *imageReader = new QImageReader(path);
    QSize imageSize = imageReader->size();

    this->fullPath = path;
    this->fileName = fileInfo.fileName();
    this->size = fileInfo.size();
    this->compressedSize = this->size;

    this->width = imageSize.width();
    this->height = imageSize.height();
    this->compressedWidth = this->width;
    this->compressedHeight = this->height;

    this->status = CImageStatus::UNCOMPRESSED;

    delete imageReader;
}

bool operator== (const CImage &c1, const CImage &c2)
{
    return (c1.fullPath == c2.fullPath);
}

bool operator!= (const CImage &c1, const CImage &c2)
{
    return !(c1 == c2);
}

QString CImage::getFormattedSize()
{
    size_t size = this->status == CImageStatus::COMPRESSED ? this->compressedSize : this->size;
    return toHumanSize(size);
}

QString CImage::getResolution()
{
    if (this->status == CImageStatus::COMPRESSED) {
        return QString::number(this->compressedWidth) + "x" + QString::number(this->compressedHeight);
    } else {
        return QString::number(this->width) + "x" + QString::number(this->height);
    }
}

QString CImage::getFileName() const
{
    return fileName;
}

QString CImage::getFullPath() const
{
    return this->fullPath;
}


bool CImage::compress()
{
    QSettings settings;
    QString outputPath = settings.value("compression_options/output/output_folder").toString();
    QString suffix = settings.value("compression_options/output/output_suffix").toString();

    QDir outputDir(outputPath);
    if (!outputDir.exists()) {
        if(!outputDir.mkpath(outputPath)) {
            return false;
        }
    }

    QFileInfo fileInfo = QFileInfo(this->getFileName());
    QString fullFileName = fileInfo.baseName() + suffix + "." + fileInfo.completeSuffix();
    QString outPath = outputDir.absoluteFilePath(fullFileName);

    int compressionLevel = settings.value("compression_options/compression/level", 4).toBool();
    bool lossless = settings.value("compression_options/compression/lossless", false).toBool();
    bool keepMetadata = settings.value("compression_options/compression/keep_metadata", false).toBool();
    int res = 0;
    cs_image_pars compress_pars = getCompressionParametersFromLevel(compressionLevel, lossless, keepMetadata);
    bool result = cs_compress(this->getFullPath().toUtf8().constData(), outPath.toUtf8().constData(), &compress_pars, &res);
    if (result) {
        this->setCompressedInfo(outPath);
    }
    return result;
}

void CImage::setCompressedInfo(QString outputPath)
{
    QFileInfo fileInfo = QFileInfo(outputPath);
    this->compressedSize = fileInfo.size();
    this->compressedFullPath = outputPath;
}

QString CImage::getCompressedFullPath() const
{
    return compressedFullPath;
}

double CImage::getRatio() const
{
    return (double)this->compressedSize / this->size;
}

QString CImage::getFormattedSavedRatio()
{
    return QString::number(round(100 - (this->getRatio() * 100))) + "%";
}

CImageStatus CImage::getStatus() const
{
    return status;
}

void CImage::setStatus(const CImageStatus &value)
{
    status = value;
}
