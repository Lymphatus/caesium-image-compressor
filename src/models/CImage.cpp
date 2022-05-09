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
    this->directory = fileInfo.canonicalPath();
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
    bool needsFormatting = this->status == CImageStatus::COMPRESSED || this->status == CImageStatus::WARNING;
    size_t s = needsFormatting ? this->compressedSize : this->size;
    return toHumanSize((double)s);
}

QString CImage::getRichFormattedSize()
{
    bool needsFormatting = this->status == CImageStatus::COMPRESSED || this->status == CImageStatus::WARNING;
    if (needsFormatting && this->size != this->compressedSize) {
        return "<small><s>" + toHumanSize((double)this->size) + "</s></small> " + toHumanSize((double)this->compressedSize);
    }
    return toHumanSize((double)this->size);
}

QString CImage::getResolution()
{
    bool needsFormatting = this->status == CImageStatus::COMPRESSED || this->status == CImageStatus::WARNING;
    if (needsFormatting) {
        return QString::number(this->compressedWidth) + "x" + QString::number(this->compressedHeight);
    }
    return QString::number(this->width) + "x" + QString::number(this->height);
}

QString CImage::getRichResolution()
{
    bool needsFormatting = this->status == CImageStatus::COMPRESSED || this->status == CImageStatus::WARNING;
    if (needsFormatting && (this->width != this->compressedWidth || this->height != this->compressedHeight)) {
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
    QString outputPath = compressionOptions.sameFolderAsInput ? this->getDirectory() : compressionOptions.outputPath;
    QString inputFullPath = this->getFullPath();
    QString suffix = compressionOptions.suffix;
    QFileInfo inputFileInfo = QFileInfo(inputFullPath);
    this->additionalInfo = "";
    if (!inputFileInfo.exists()) {
        qCritical() << "File" << inputFullPath << "does not exist.";
        this->additionalInfo = QIODevice::tr("Input file does not exist");
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
            qCritical() << "Cannot make path" << outputPath;
            this->additionalInfo = QIODevice::tr("Cannot make output path, check your permissions");
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
        qCritical() << "Temporary file" << tempFileFullPath << "is empty.";
        this->additionalInfo = QIODevice::tr("Temporary file creation failed");
        return false;
    }

    tempFile.close();

    bool lossless = compressionOptions.lossless;
    bool keepMetadata = compressionOptions.keepMetadata;

    CCSParameters r_parameters = {
        keepMetadata,
        static_cast<unsigned int>(compressionOptions.jpeg_quality),
        static_cast<unsigned int>(compressionOptions.png_level),
        false,
        20,
        static_cast<unsigned int>(compressionOptions.webp_quality),
        lossless,
        0,
        0
    };

    // Resize
    if (compressionOptions.resize) {
        QImageReader imageReader(this->getFullPath());
        QSize originalSize = imageReader.size();

        std::tuple<unsigned int, unsigned int> dimensions = cResize(originalSize,
            compressionOptions.fitTo,
            compressionOptions.width,
            compressionOptions.height,
            compressionOptions.size,
            compressionOptions.doNotEnlarge);

        if (std::get<0>(dimensions) != originalSize.width() || std::get<1>(dimensions) != originalSize.height()) {
            r_parameters.width = std::get<0>(dimensions);
            r_parameters.height = std::get<1>(dimensions);
        }
    }

    CCSResult result = c_compress(inputFullPath.toUtf8().constData(), tempFileFullPath.toUtf8().constData(), r_parameters);
    if (result.success) {
        QFileInfo outputInfo(tempFileFullPath);

        bool outputIsBiggerThanInput = outputInfo.size() >= inputFileInfo.size();

        if (outputAlreadyExists && !outputIsBiggerThanInput) {
            QFile::remove(outputFullPath);
        }
        QString inputCopyFile = inputFullPath;

        if (!outputIsBiggerThanInput) {
            inputCopyFile = tempFileFullPath;
        } else {
            this->status = CImageStatus::WARNING;
            this->additionalInfo = QIODevice::tr("Skipped: compressed file is bigger than original");
            QFileInfo outputFileInfo = QFileInfo(inputFullPath);
            this->setCompressedInfo(outputFileInfo);
            return true;
        }
        bool copyResult = QFile::copy(inputCopyFile, outputFullPath);

        if (!copyResult) {
            qCritical() << "Failed to copy from" << inputCopyFile << "to" << outputFullPath;
            this->additionalInfo = QIODevice::tr("Cannot copy output file, check your permissions");
            return false;
        }
        QFileInfo outputFileInfo = QFileInfo(outputFullPath);
        if (compressionOptions.keepDates) {
            this->setFileDates(outputFileInfo, compressionOptions, inputFileDates);
        }
        this->setCompressedInfo(outputFileInfo);
    } else {
        this->additionalInfo = result.error_message;
        qCritical() << "Compression result for i:" << inputFullPath << "and o: "<< tempFileFullPath << "is false. Error:" << result.error_message;
    }

    return result.success;
}

void CImage::setCompressedInfo(QFileInfo fileInfo)
{
    QImage compressedImage(fileInfo.canonicalFilePath());
    this->compressedDirectory = fileInfo.canonicalPath();
    this->compressedSize = fileInfo.size();
    this->compressedFullPath = fileInfo.canonicalFilePath();
    this->compressedWidth = compressedImage.width();
    this->compressedHeight = compressedImage.height();
}

void CImage::setFileDates(QFileInfo fileInfo, CompressionOptions compressionOptions, FileDates inputFileDates)
{
    QFile outputFile(fileInfo.canonicalFilePath());
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

QString CImage::getFormattedStatus() const
{
    switch (this->status) {
    case CImageStatus::COMPRESSING:
        return QIODevice::tr("Compressing...");
    case CImageStatus::ERROR:
        return QIODevice::tr("Error:") + " " + this->additionalInfo;
    case CImageStatus::WARNING:
        return this->additionalInfo;
    case CImageStatus::COMPRESSED:
    case CImageStatus::UNCOMPRESSED:
    default:
        return "";
    }
}

QString CImage::getDirectory() const
{
    return this->directory;
}

QString CImage::getCompressedDirectory() const
{
    return this->compressedDirectory;
}
