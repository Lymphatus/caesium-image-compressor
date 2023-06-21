#include "CImage.h"

#include "./exceptions/ImageNotSupportedException.h"
#include "exceptions/ImageTooBigException.h"
#include <QDir>
#include <QImageReader>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <cmath>

CImage::CImage(const QString& path)
{
    QFileInfo fileInfo = QFileInfo(path);
    auto* imageReader = new QImageReader();
    imageReader->setAutoDetectImageFormat(true);
    imageReader->setAutoTransform(true);
    imageReader->setFileName(path);
    this->format = imageReader->format().toLower();
    if (this->format == "jpeg") {
        this->format = "jpg";
    }

    if (!supportedFormats.contains(format)) {
        delete imageReader;
        throw ImageNotSupportedException();
    }

    this->extension = fileInfo.suffix();
    this->size = fileInfo.size();

    if (this->size > 300 * 1024 * 1024) {
        throw ImageTooBigException();
    }

    this->fullPath = fileInfo.canonicalFilePath();
    this->directory = fileInfo.canonicalPath();
    this->fileName = fileInfo.fileName();
    this->compressedSize = this->size;

    QSize imageSize = getSizeWithMetadata(imageReader);
    this->width = imageSize.width();
    this->height = imageSize.height();

    this->compressedWidth = this->width;
    this->compressedHeight = this->height;

    this->hashedFullPath = hashString(this->fullPath, QCryptographicHash::Sha256);

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

bool CImage::preview(const CompressionOptions& compressionOptions)
{
    QString inputFullPath = this->fullPath;
    QFileInfo inputFileInfo(inputFullPath);
    QString outputFullPath = this->getTemporaryPreviewFullPath();
    const QString& outputFormat = getOutputSupportedFormats()[compressionOptions.format];
    bool convert = compressionOptions.format != 0 && this->getFormat().compare(outputFormat, Qt::CaseInsensitive) != 0;
    FileDates inputFileDates = {
        inputFileInfo.fileTime(QFile::FileBirthTime),
        inputFileInfo.fileTime(QFile::FileModificationTime),
        inputFileInfo.fileTime(QFile::FileAccessTime)
    };
    CCSParameters r_parameters = this->getCSParameters(compressionOptions);
    if (convert) {
        QImage imageToBeConverted = QImage(inputFullPath);
        imageToBeConverted.save(outputFullPath, getOutputSupportedFormats().at(compressionOptions.format).toLower().toUtf8().constData(), 100);
        inputFullPath = outputFullPath;
    }
    CCSResult result = c_compress(inputFullPath.toUtf8().constData(), outputFullPath.toUtf8().constData(), r_parameters);

    QFileInfo outputFileInfo(outputFullPath);
    this->setFileDates(outputFileInfo, compressionOptions.datesMap, inputFileDates);
    return result.success;
}

bool CImage::compress(const CompressionOptions& compressionOptions)
{
    QString outputPath = compressionOptions.sameFolderAsInput ? this->getDirectory() : compressionOptions.outputPath;
    QString inputFullPath = this->getFullPath();
    QString suffix = compressionOptions.suffix;
    QFileInfo inputFileInfo = QFileInfo(inputFullPath);
    QString outputFormat = getOutputSupportedFormats()[compressionOptions.format];
    bool convert = compressionOptions.format != 0 && this->format.compare(outputFormat, Qt::CaseInsensitive) != 0;
    this->additionalInfo = "";
    if (!inputFileInfo.exists()) {
        qCritical() << "File" << inputFullPath << "does not exist.";
        this->additionalInfo = QIODevice::tr("Input file does not exist");
        return false;
    }
    QString outputSuffix = this->extension;
    if (compressionOptions.format != 0) {
        outputSuffix = getOutputSupportedFormats()[compressionOptions.format].toLower();
    }

    QString fullFileName = inputFileInfo.completeBaseName() + suffix + "." + outputSuffix;
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

    QString tempFileFullPath = "";

    QString outputFullPath = outputDir.absoluteFilePath(fullFileName);
    bool outputAlreadyExists = QFile(outputFullPath).exists();

    QTemporaryFile tempFile(outputPath + QDir::separator() + inputFileInfo.completeBaseName() + ".XXXXXXXX." + outputSuffix);
    if (tempFile.open()) {
        tempFileFullPath = tempFile.fileName();
    }

    if (tempFileFullPath.isEmpty()) {
        qCritical() << "Temporary file" << tempFileFullPath << "is empty.";
        this->additionalInfo = QIODevice::tr("Temporary file creation failed");
        return false;
    }

    tempFile.close();

    QString inputCopyFile = inputFullPath;
    if (convert) {
        QImage imageToBeConverted = QImage(inputFullPath);
        imageToBeConverted.save(tempFileFullPath, getOutputSupportedFormats().at(compressionOptions.format).toLower().toUtf8().constData(), 100);
        inputFullPath = tempFileFullPath;
    }

    QString previewPath = this->getTemporaryPreviewFullPath();
    if (QFile::exists(previewPath)) {
        bool removeSuccess = QFile::remove(tempFileFullPath);
        if (removeSuccess) {
            QFile::copy(previewPath, tempFileFullPath);
        }
    }

    CCSParameters r_parameters = this->getCSParameters(compressionOptions);

    CCSResult result = c_compress(inputFullPath.toUtf8().constData(), tempFileFullPath.toUtf8().constData(), r_parameters);
    if (result.success) {
        QFileInfo outputInfo(tempFileFullPath);

        bool outputIsBiggerThanInput = outputInfo.size() >= inputFileInfo.size() && compressionOptions.skipIfBigger;
        bool inputAlreadyMovedToTrash = false;
        if (outputAlreadyExists && !outputIsBiggerThanInput) {
            if (compressionOptions.sameFolderAsInput) {
                bool trashingResult = QFile::moveToTrash(outputFullPath);
                // Can fail in some conditions, like NAS storages. Fallback to normal remove.
                if (!trashingResult) {
                    QFile::remove(outputFullPath);
                }
                inputAlreadyMovedToTrash = true;
            } else {
                QFile::remove(outputFullPath);
            }
        }

        if (!outputIsBiggerThanInput) {
            inputCopyFile = tempFileFullPath;
        } else {
            this->status = CImageStatus::WARNING;
            this->additionalInfo = QIODevice::tr("Skipped: compressed file is bigger than original");
            if (outputAlreadyExists) {
                QFileInfo outputFileInfo = QFileInfo(outputFullPath);
                this->setCompressedInfo(outputFileInfo);
                return true;
            }
        }
        bool copyResult = QFile::copy(inputCopyFile, outputFullPath);
        if (!copyResult) {
            qCritical() << "Failed to copy from" << inputCopyFile << "to" << outputFullPath;
            this->additionalInfo = QIODevice::tr("Cannot copy output file, check your permissions");
            return false;
        }
        if (compressionOptions.moveToTrash && !inputAlreadyMovedToTrash) {
            bool trashingResult = QFile::moveToTrash(this->getFullPath());
            if (!trashingResult) {
                qWarning() << "Cannot move to trash file" << this->getFullPath();
                this->additionalInfo = QIODevice::tr("Cannot move original file to trash, check your permissions");
            }
        }

        QFileInfo outputFileInfo = QFileInfo(outputFullPath);
        if (compressionOptions.keepDates) {
            this->setFileDates(outputFileInfo, compressionOptions.datesMap, inputFileDates);
        }
        this->setCompressedInfo(outputFileInfo);
    } else {
        this->additionalInfo = result.error_message;
        qCritical() << "Compression result for i:" << inputFullPath << "and o: " << tempFileFullPath << "is false. Error:" << result.error_message;
    }

    return result.success;
}

CCSParameters CImage::getCSParameters(const CompressionOptions& compressionOptions)
{
    bool lossless = compressionOptions.lossless;
    bool keepMetadata = compressionOptions.keepMetadata;

    CCSParameters r_parameters = {
        keepMetadata,
        static_cast<unsigned int>(compressionOptions.jpeg_quality),
        static_cast<unsigned int>(compressionOptions.png_quality),
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
    return r_parameters;
}

void CImage::setCompressedInfo(QFileInfo fileInfo)
{
    QImageReader imageReader(fileInfo.canonicalFilePath());
    QSize imageSize = getSizeWithMetadata(&imageReader);
    this->compressedDirectory = fileInfo.canonicalPath();
    this->compressedSize = fileInfo.size();
    this->compressedFullPath = fileInfo.canonicalFilePath();
    this->compressedWidth = imageSize.width();
    this->compressedHeight = imageSize.height();
}

void CImage::setFileDates(QFileInfo fileInfo, FileDatesOutputOption datesMap, FileDates inputFileDates)
{
    QFile outputFile(fileInfo.canonicalFilePath());
    outputFile.open(QIODevice::ReadWrite);
    if (datesMap.keepCreation) {
        outputFile.setFileTime(inputFileDates.creation, QFileDevice::FileBirthTime);
    }
    if (datesMap.keepLastModified) {
        outputFile.setFileTime(inputFileDates.lastModified, QFileDevice::FileModificationTime);
    }
    if (datesMap.keepLastAccess) {
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

QString CImage::getHashedFullPath() const
{
    return this->hashedFullPath;
}

QString CImage::getTemporaryPreviewFullPath() const
{
    QString tempFileName = hashString(this->hashedFullPath + "." + getCompressionOptionsHash(), QCryptographicHash::Sha256);
    QString cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QString temporaryPreviewFullPath = cacheDir + QDir::separator() + tempFileName;
    QDir(cacheDir).mkpath(cacheDir);
    return temporaryPreviewFullPath + ".jpg";
}

QString CImage::getPreviewFullPath() const
{
    if (!this->compressedFullPath.isEmpty()) {
        return this->compressedFullPath;
    }

    return this->getTemporaryPreviewFullPath();
}

QString CImage::getFormat() const
{
    return this->format;
}

QSize CImage::getSizeWithMetadata(QImageReader* reader)
{
    QSize imageSize = reader->size();
    QSize actualSize(imageSize.width(), imageSize.height());
    QFlags<QImageIOHandler::Transformation> transformation = reader->transformation();
    // We need to check if the image is rotated by metadata and adjust the values accordingly
    if (transformation == QImageIOHandler::TransformationRotate90
        || transformation == QImageIOHandler::TransformationMirrorAndRotate90
        || transformation == QImageIOHandler::TransformationFlipAndRotate90
        || transformation == QImageIOHandler::TransformationRotate270) {
        actualSize.setWidth(imageSize.height());
        actualSize.setHeight(imageSize.width());
    }

    return actualSize;
}
