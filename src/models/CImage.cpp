#include "CImage.h"

#include <QImageReader>

#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QTemporaryFile>


CImage::CImage(const QString& path)
{
    QFileInfo fileInfo = QFileInfo(path);
    auto *imageReader = new QImageReader(path);
    QSize imageSize = imageReader->size();

    this->fullPath = fileInfo.absoluteFilePath();
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


bool CImage::compress(CompressionOptions compressionOptions)
{
    QSettings settings;
    QString outputPath = compressionOptions.outputPath;
    QString inputFullPath = this->getFullPath();
    QString suffix = compressionOptions.suffix;
    QFileInfo inputFileInfo = QFileInfo(inputFullPath);
    QString fullFileName = inputFileInfo.baseName() + suffix + "." + inputFileInfo.completeSuffix();

    if (compressionOptions.keepStructure) {
        outputPath = inputFileInfo.absolutePath().remove(0, compressionOptions.basePath.length());
        outputPath = QDir::cleanPath(compressionOptions.outputPath + QDir::separator() + outputPath);
    }

    QDir outputDir(outputPath);
    if (!outputDir.exists()) {
        if(!outputDir.mkpath(outputPath)) {
            return false;
        }
    }

    QString outputFullPath = outputDir.absoluteFilePath(fullFileName);

    QString tempFileFullPath = "";
    bool outputAlreadyExists = QFile(outputFullPath).exists();

    if (outputAlreadyExists) {
        QTemporaryFile tempFile;
        if (tempFile.open()) {
            tempFileFullPath = tempFile.fileName();
        }

        if (tempFileFullPath.isEmpty()) {
            return false;
        }
        tempFile.close();
    }

    if (tempFileFullPath.isEmpty()) {
        tempFileFullPath = outputFullPath;
    }

    int compressionLevel = compressionOptions.compressionLevel;
    bool lossless = compressionOptions.lossless;
    bool keepMetadata = compressionOptions.keepMetadata;
    int res = 0;
    cs_image_pars compress_pars = getCompressionParametersFromLevel(compressionLevel, lossless, keepMetadata);

    bool result = cs_compress(this->getFullPath().toUtf8().constData(), tempFileFullPath.toUtf8().constData(), &compress_pars, &res);
    if (result) {
        QFileInfo outputInfo(tempFileFullPath);
        if (outputAlreadyExists && outputInfo.size() < inputFileInfo.size()) {
            QFile::remove(outputFullPath);
            bool copyResult = QFile::copy(tempFileFullPath, outputFullPath);
            if (!copyResult) {
                return false;
            }
        }
        this->setCompressedInfo(outputFullPath);
    }
    return result;
}

void CImage::setCompressedInfo(QFileInfo fileInfo)
{
    this->compressedSize = fileInfo.size();
    this->compressedFullPath = fileInfo.absoluteFilePath();
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
