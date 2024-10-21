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

    if (this->size > 500 * 1024 * 1024) {
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

QString CImage::getFormattedSize() const
{
    bool needsFormatting = this->status == CImageStatus::COMPRESSED || this->status == CImageStatus::WARNING;
    size_t s = needsFormatting ? this->compressedSize : this->size;
    return toHumanSize(static_cast<double>(s));
}

QString CImage::getRichFormattedSize() const
{
    bool needsFormatting = this->status == CImageStatus::COMPRESSED || this->status == CImageStatus::WARNING;
    if (needsFormatting && this->size != this->compressedSize) {
        return "<small><s>" + toHumanSize(static_cast<double>(this->size)) + "</s></small> " + toHumanSize(static_cast<double>(this->compressedSize));
    }
    return toHumanSize(static_cast<double>(this->size));
}

QString CImage::getResolution() const
{
    if (this->status == CImageStatus::COMPRESSED || this->status == CImageStatus::WARNING) {
        return QString::number(this->compressedWidth) + "x" + QString::number(this->compressedHeight);
    }
    return QString::number(this->width) + "x" + QString::number(this->height);
}

QString CImage::getRichResolution() const
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

bool CImage::preview(const CompressionOptions& compressionOptions) const
{
    QString inputFullPath = this->fullPath;
    QFileInfo inputFileInfo(inputFullPath);
    QString outputFullPath = this->getTemporaryPreviewFullPath();
    if (outputFullPath.isEmpty()) {
        return false;
    }
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
        bool conversionSuccess = imageToBeConverted.save(outputFullPath, getOutputSupportedFormats().at(compressionOptions.format).toLower().toUtf8().constData(), 100);
        if (!conversionSuccess) {
            return false;
        }
        inputFullPath = outputFullPath;
    }
    size_t maxOutputSize = getMaxOutputSizeInBytes(compressionOptions.maxOutputSize, inputFileInfo.size());

    CCSResult result = compressionOptions.compressionMode == SIZE ? c_compress_to_size(inputFullPath.toUtf8().constData(), outputFullPath.toUtf8().constData(), r_parameters, maxOutputSize) : c_compress(inputFullPath.toUtf8().constData(), outputFullPath.toUtf8().constData(), r_parameters);

    QFileInfo outputFileInfo(outputFullPath);
    CImage::setFileDates(outputFileInfo, compressionOptions.datesMap, inputFileDates);
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
    QString fullFileNameWithOriginalExtension = inputFileInfo.completeBaseName() + suffix + "." + this->extension;
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
        bool conversionSuccess = imageToBeConverted.save(tempFileFullPath, getOutputSupportedFormats().at(compressionOptions.format).toLower().toUtf8().constData(), 100);
        this->additionalInfo = QIODevice::tr("File conversion failed");
        if (!conversionSuccess) {
            return false;
        }
        inputFullPath = tempFileFullPath;
    }

    QString previewPath = this->getTemporaryPreviewFullPath();
    if (QFile::exists(previewPath)) {
        if (QFile::remove(tempFileFullPath)) {
            QFile::copy(previewPath, tempFileFullPath);
        }
    }

    CCSParameters r_parameters = this->getCSParameters(compressionOptions);

    size_t maxOutputSize = getMaxOutputSizeInBytes(compressionOptions.maxOutputSize, inputFileInfo.size());

    CCSResult result = compressionOptions.compressionMode == SIZE ? c_compress_to_size(inputFullPath.toUtf8().constData(), tempFileFullPath.toUtf8().constData(), r_parameters, maxOutputSize) : c_compress(inputFullPath.toUtf8().constData(), tempFileFullPath.toUtf8().constData(), r_parameters);

    if (result.success) {
        QFileInfo outputInfo(tempFileFullPath);

        bool outputIsBiggerThanInput = outputInfo.size() >= inputFileInfo.size() && compressionOptions.skipIfBigger;
        bool inputAlreadyMoved = false;

        // If the output is bigger and we are converting, we should fallback to the original file with original extension
        if (outputIsBiggerThanInput && convert) {
            outputFullPath = outputDir.absoluteFilePath(fullFileNameWithOriginalExtension);
            outputAlreadyExists = QFile(outputFullPath).exists();
        }

        if (outputAlreadyExists && !outputIsBiggerThanInput) {
            if (compressionOptions.sameFolderAsInput) {
                bool trashingResult = false;
                if (!compressionOptions.moveOriginalFile || (compressionOptions.moveOriginalFile && compressionOptions.moveOriginalFileDestination == 0)) {
                    trashingResult = QFile::moveToTrash(outputFullPath);
                }
                // Can fail in some conditions, like NAS storages. Fallback to normal remove.
                if (!trashingResult) {
                    QFile::remove(outputFullPath);
                }
                inputAlreadyMoved = true;
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
        if (compressionOptions.moveOriginalFile && !inputAlreadyMoved) {
            if (compressionOptions.moveOriginalFileDestination == 0 && !QFile::moveToTrash(this->getFullPath())) {
                qWarning() << "Cannot move to trash file" << this->getFullPath();
                this->additionalInfo = QIODevice::tr("Cannot move original file to trash, check your permissions");
            } else if (compressionOptions.moveOriginalFileDestination == 1 && !QFile::remove(this->getFullPath())) {
                qWarning() << "Cannot delete file" << this->getFullPath();
                this->additionalInfo = QIODevice::tr("Cannot delete original file, check your permissions");
            }
        }

        QFileInfo outputFileInfo = QFileInfo(outputFullPath);
        if (compressionOptions.keepDates) {
            CImage::setFileDates(outputFileInfo, compressionOptions.datesMap, inputFileDates);
        }
        this->setCompressedInfo(outputFileInfo);
    } else {
        this->additionalInfo = result.error_message;
        qCritical() << "Compression result for i:" << inputFullPath << "and o: " << tempFileFullPath << "is false. Error:" << result.error_message;
    }

    return result.success;
}

CCSParameters CImage::getCSParameters(const CompressionOptions& compressionOptions) const
{
    CCSParameters r_parameters = {};
    r_parameters.keep_metadata = compressionOptions.keepMetadata;
    r_parameters.jpeg_quality = static_cast<unsigned int>(compressionOptions.jpegQuality);
    r_parameters.jpeg_chroma_subsampling = compressionOptions.jpegChromaSubsampling;
    r_parameters.jpeg_progressive = compressionOptions.jpegProgressive;
    r_parameters.png_quality = static_cast<unsigned int>(compressionOptions.pngQuality);
    r_parameters.png_optimization_level = static_cast<unsigned int>(compressionOptions.pngOptimizationLevel);
    r_parameters.png_force_zopfli = false;
    r_parameters.gif_quality = 20;
    r_parameters.webp_quality = static_cast<unsigned int>(compressionOptions.webpQuality);
    r_parameters.tiff_compression = static_cast<unsigned int>(compressionOptions.tiffMethod);
    r_parameters.tiff_deflate_level = static_cast<unsigned int>(compressionOptions.tiffDeflateLevel);
    r_parameters.optimize = compressionOptions.lossless;
    r_parameters.width = 0;
    r_parameters.height = 0;

    // Resize
    if (compressionOptions.resize) {
        QImageReader imageReader(this->getFullPath());
        QSize originalSize = imageReader.size();

        std::tuple<unsigned int, unsigned int> dimensions = cResize(&imageReader, compressionOptions);

        if (std::get<0>(dimensions) != originalSize.width() || std::get<1>(dimensions) != originalSize.height()) {
            r_parameters.width = static_cast<int>(std::get<0>(dimensions));
            r_parameters.height = static_cast<int>(std::get<1>(dimensions));
        }
    }
    return r_parameters;
}

void CImage::setCompressedInfo(const QFileInfo& fileInfo)
{
    QImageReader imageReader(fileInfo.canonicalFilePath());
    QSize imageSize = getSizeWithMetadata(&imageReader);
    this->compressedDirectory = fileInfo.canonicalPath();
    this->compressedSize = fileInfo.size();
    this->compressedFullPath = fileInfo.canonicalFilePath();
    this->compressedWidth = imageSize.width();
    this->compressedHeight = imageSize.height();
}

void CImage::setFileDates(const QFileInfo& fileInfo, FileDatesOutputOption datesMap, const FileDates& inputFileDates)
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
    return static_cast<double>(this->compressedSize) / static_cast<double>(this->size);
}

QString CImage::getFormattedSavedRatio() const
{
    return QString::number(round(100 - (this->getRatio() * 100))) + "%";
}

QString CImage::getRichFormattedSavedRatio() const
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
    bool pathCreationSuccess = QDir(cacheDir).mkpath(cacheDir);
    if (!pathCreationSuccess) {
        return { "" };
    }
    return temporaryPreviewFullPath;
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

size_t CImage::getMaxOutputSizeInBytes(MaxOutputSize maxOutputSize, size_t originalSize)
{
    if (maxOutputSize.unit == MAX_OUTPUT_PERCENTAGE) {
        return std::floor(originalSize * maxOutputSize.maxOutputSize / 100);
    }

    return maxOutputSize.maxOutputSize << (maxOutputSize.unit * 10);
}
