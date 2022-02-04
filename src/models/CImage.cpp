#include "CImage.h"

#include "./exceptions/ImageNotSupportedException.h"
#include "exceptions/ImageTooBigException.h"
#include <QDir>
#include <QImageReader>
#include <QSettings>
#include <QTemporaryFile>
#include <cmath>

CImage::CImage(const QString& path)
{
    QFileInfo fileInfo = QFileInfo(path);
    auto* imageReader = new QImageReader(path);
    auto format = imageReader->format().toLower();

    if (!supportedFormats.contains(format)) {
        throw ImageNotSupportedException();
    }

    this->size = fileInfo.size();

    if (this->size > 104857600) {
        throw ImageTooBigException();
    }

    this->fullPath = fileInfo.canonicalFilePath();
    this->fileName = fileInfo.fileName();
    this->compressedSize = this->size;

    QSize imageSize = imageReader->size();
    this->width = imageSize.width();
    this->height = imageSize.height();
    this->compressedWidth = this->width;
    this->compressedHeight = this->height;

    this->status = CImageStatus::UNCOMPRESSED;

    delete imageReader;
}

bool operator==(const CImage& c1, const CImage& c2)
{
    return (c1.fullPath == c2.fullPath);
}

bool operator!=(const CImage& c1, const CImage& c2)
{
    return !(c1 == c2);
}

QString CImage::getFormattedSize()
{
    size_t s = this->status == CImageStatus::COMPRESSED ? this->compressedSize : this->size;
    return toHumanSize((double)s);
}

QString CImage::getRichFormattedSize()
{
    if (this->status == CImageStatus::COMPRESSED && this->size != this->compressedSize) {
        return "<small><s>" + toHumanSize((double)this->size) + "</s></small> " + toHumanSize((double)this->compressedSize);
    }
    return toHumanSize((double)this->size);
}

QString CImage::getResolution()
{
    if (this->status == CImageStatus::COMPRESSED) {
        return QString::number(this->compressedWidth) + "x" + QString::number(this->compressedHeight);
    }
    return QString::number(this->width) + "x" + QString::number(this->height);
}

QString CImage::getRichResolution()
{
    if (this->status == CImageStatus::COMPRESSED && (this->width != this->compressedWidth || this->height != this->compressedHeight)) {
        return "<small><s>" + QString::number(this->width) + "x" + QString::number(this->height) + "</s></small> " + QString::number(this->compressedWidth) + "x" + QString::number(this->compressedHeight);
    }
    return QString::number(this->width) + "x" + QString::number(this->height);
}

QString CImage::getFileName() const
{
    return fileName;
}

QString CImage::getFullPath() const
{
    return this->fullPath;
}

bool CImage::compress(const CompressionOptions& compressionOptions)
{
    QSettings settings;
    QString outputPath = compressionOptions.sameFolderAsInput ? QFileInfo(this->getFullPath()).absoluteDir().absolutePath() : compressionOptions.outputPath;
    QString inputFullPath = this->getFullPath();
    QString suffix = compressionOptions.suffix;
    QFileInfo inputFileInfo = QFileInfo(inputFullPath);
    if (!inputFileInfo.exists()) {
        return false;
    }
    QString fullFileName = inputFileInfo.completeBaseName() + suffix + "." + inputFileInfo.suffix();
    FileDates inputFileDates = {
        inputFileInfo.fileTime(QFile::FileBirthTime),
        inputFileInfo.fileTime(QFile::FileModificationTime),
        inputFileInfo.fileTime(QFile::FileAccessTime)
    };

    if (compressionOptions.keepStructure && !compressionOptions.sameFolderAsInput) {
        outputPath = inputFileInfo.absolutePath().remove(0, compressionOptions.basePath.length());
        outputPath = QDir::cleanPath(compressionOptions.outputPath + QDir::separator() + outputPath);
    }

    QDir outputDir(outputPath);
    if (!outputDir.exists()) {
        if (!outputDir.mkpath(outputPath)) {
            return false;
        }
    }

    QString outputFullPath = outputDir.absoluteFilePath(fullFileName);

    QString tempFileFullPath = "";
    bool outputAlreadyExists = QFile(outputFullPath).exists();

    QTemporaryFile tempFile(outputPath + QDir::separator() + inputFileInfo.completeBaseName() + ".XXXXXXXX." + inputFileInfo.suffix());
    if (tempFile.open()) {
        tempFileFullPath = tempFile.fileName();
    }

    if (tempFileFullPath.isEmpty()) {
        return false;
    }

    tempFile.close();

    bool lossless = compressionOptions.lossless;
    bool keepMetadata = compressionOptions.keepMetadata;

    C_CSParameters r_parameters = {
        keepMetadata,
        static_cast<unsigned int>(compressionOptions.jpeg_quality),
        static_cast<unsigned int>(compressionOptions.png_level),
        false,
        20,
        static_cast<unsigned int>(compressionOptions.webp_quality),
        lossless
    };

    QImageReader::setAllocationLimit(512);
    // Resize
    if (compressionOptions.resize) {
        QImage image(this->getFullPath());
        QSize originalSize = image.size();

        image = cResize(image,
            compressionOptions.fitTo,
            compressionOptions.width,
            compressionOptions.height,
            compressionOptions.size,
            compressionOptions.doNotEnlarge);

        if (image.size() != originalSize) {
            bool saveResult = image.save(tempFileFullPath, inputFileInfo.suffix().toLocal8Bit(), 100);
            if (!saveResult) {
                return false;
            }
            inputFullPath = tempFileFullPath;
        }
    }

    bool result = c_compress(inputFullPath.toUtf8().constData(), tempFileFullPath.toUtf8().constData(), r_parameters);
    if (result) {
        if (keepMetadata) {
            bool metadataCopyResult = copyMetadata(inputFullPath.toUtf8().constData(), tempFileFullPath.toUtf8().constData());
            if (!metadataCopyResult) {
                return false;
            }
        }
        QFileInfo outputInfo(tempFileFullPath);

        bool outputIsBiggerThanInput = outputInfo.size() >= inputFileInfo.size();
        bool copyResult;

        if (outputAlreadyExists) {
            QFile::remove(outputFullPath);
        }

        if (!outputIsBiggerThanInput) {
            copyResult = QFile::copy(tempFileFullPath, outputFullPath);
        } else {
            copyResult = QFile::copy(inputFullPath, outputFullPath);
        }

        if (!copyResult) {
            return false;
        }
        QFileInfo outputFileInfo = QFileInfo(outputFullPath);
        if (compressionOptions.keepDates) {
            this->setFileDates(outputFileInfo, compressionOptions, inputFileDates);
        }
        this->setCompressedInfo(outputFileInfo);
    }
    return result;
}

void CImage::setCompressedInfo(QFileInfo fileInfo)
{
    QImage compressedImage(fileInfo.absoluteFilePath());
    this->compressedSize = fileInfo.size();
    this->compressedFullPath = fileInfo.absoluteFilePath();
    this->compressedWidth = compressedImage.width();
    this->compressedHeight = compressedImage.height();
}

void CImage::setFileDates(QFileInfo fileInfo, CompressionOptions compressionOptions, FileDates inputFileDates)
{
    QFile outputFile(fileInfo.absoluteFilePath());
    outputFile.open(QIODevice::ReadWrite);
    if (compressionOptions.datesMap.keepCreation) {
        outputFile.setFileTime(inputFileDates.creation, QFileDevice::FileBirthTime);
    }
    if (compressionOptions.datesMap.keepLastModified) {
        outputFile.setFileTime(inputFileDates.lastModified, QFileDevice::FileModificationTime);
    }
    if (compressionOptions.datesMap.keepLastAccess) {
        outputFile.setFileTime(inputFileDates.lastAccess, QFileDevice::FileAccessTime);
    }
    outputFile.close();
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

QString CImage::getRichFormattedSavedRatio()
{
    // TODO
    return this->getFormattedSavedRatio();
}

CImageStatus CImage::getStatus() const
{
    return status;
}

void CImage::setStatus(const CImageStatus& value)
{
    status = value;
}

size_t CImage::getOriginalSize() const
{
    return this->size;
}

size_t CImage::getCompressedSize() const
{
    return this->compressedSize;
}

size_t CImage::getTotalPixels() const
{
    return this->width * this->height;
}
