#include "Utils.h"
#include "widgets/QCaesiumMessageBox.h"

#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QImageReader>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <cmath>

QString toHumanSize(double size)
{
    bool isNegative = size < 0;
    if (isNegative) {
        size = abs(size);
    }
    if (size == 0) {
        return "0 bytes";
    }

    QStringList unit;
    unit << "bytes"
         << "KB"
         << "MB"
         << "GB"
         << "TB";
    double order = floor(log2(size) / 10);

    if (order > 4) {
        qWarning() << "Woah, that's huge!";
        order = 4;
    }

    return QString::number(size / (pow(1024, order)) * (isNegative ? -1 : 1), 'f', 2) + ' ' + unit[(int)order];
}

// TODO Another thread?
QStringList scanDirectory(const QString& directory, bool subfolders)
{
    QStringList inputFilterList = { "*.jpg", "*.jpeg", "*.png", "*.webp" };
    QStringList fileList = {};
    auto iteratorFlags = subfolders ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
    // Collecting all files in folder
    if (QDir(directory).exists()) {
        QDirIterator it(directory,
            inputFilterList,
            QDir::AllEntries,
            iteratorFlags);

        while (it.hasNext()) {
            it.next();
            fileList.append(it.filePath());
        }
    }

    return fileList;
}

QString getRootFolder(QList<QString> folderMap)
{
    QStringListIterator it(folderMap);
    QString rootFolderPath = folderMap.first();
    while (it.hasNext()) {
        QString newFolderPath = it.next();
        QStringList splitNewFolder = QDir::toNativeSeparators(newFolderPath).split(QDir::separator());
        QStringList splitRootFolder = QDir::toNativeSeparators(rootFolderPath).split(QDir::separator());
        QStringList splitCommonPath;

        for (int i = 0; i < (std::min)(splitNewFolder.count(), splitRootFolder.count()); i++) {
            if (QString::compare(splitNewFolder.at(i), splitRootFolder.at(i)) != 0) {
                if (i == 0) {
                    rootFolderPath = QDir::rootPath();
                } else {
                    rootFolderPath = QDir(splitCommonPath.join(QDir::separator())).absolutePath();
                }
                break;
            }
            splitCommonPath.append(splitNewFolder.at(i));
        }
        rootFolderPath = QDir(splitCommonPath.join(QDir::separator())).absolutePath();
    }

    return rootFolderPath;
}

std::tuple<unsigned int, unsigned int> cResize(QImageReader* reader, const CompressionOptions &compressionOptions)
{
    int fitTo = compressionOptions.fitTo;
    int width = compressionOptions.width;
    int height = compressionOptions.height;
    int size = compressionOptions.size;
    bool doNotEnlarge = compressionOptions.doNotEnlarge;
    bool keepMetadata = compressionOptions.keepMetadata;
    bool rotatedByMetadata = false;

    QSize originalSize = reader->size();
    if (keepMetadata && (fitTo == ResizeMode::FIXED_WIDTH || fitTo == ResizeMode::FIXED_HEIGHT)) {
        originalSize = getSizeWithMetadata(reader);
        rotatedByMetadata = isRotatedByMetadata(reader);
    }

    int originalWidth = originalSize.width();
    int originalHeight = originalSize.height();

    if (fitTo == ResizeMode::DIMENSIONS) {
        int outputWidth = width;
        int outputHeight = height;
        if (doNotEnlarge && (outputWidth >= originalWidth || outputHeight >= originalHeight)) {
            return { originalWidth, originalHeight };
        }
        return { outputWidth, outputHeight };
    } else if (fitTo == ResizeMode::PERCENTAGE) {
        int outputWidthPerc = width;
        int outputHeightPerc = height;

        if (doNotEnlarge && (outputWidthPerc >= 100 || outputHeightPerc >= 100)) {
            return { originalWidth, originalHeight };
        }

        int outputWidth = (int)round((double)originalWidth * (double)outputWidthPerc / 100);
        int outputHeight = (int)round((double)originalHeight * (double)outputHeightPerc / 100);
        return { outputWidth, outputHeight };
    } else if (fitTo == ResizeMode::FIXED_WIDTH) {
        if (doNotEnlarge && keepMetadata && width > originalWidth) {
            return { 0, 0 };
        } else if (doNotEnlarge && !keepMetadata && width > originalWidth) {
            return { originalWidth, originalHeight };
        }

        if (rotatedByMetadata && keepMetadata) {
            return { 0, width };
        }
        return { width, 0 };
    } else if (fitTo == ResizeMode::FIXED_HEIGHT) {
        if (doNotEnlarge && keepMetadata && height > originalHeight) {
            return { 0, 0 };
        } else if (doNotEnlarge && !keepMetadata && height > originalHeight) {
            return { originalWidth, originalHeight };
        }

        if (rotatedByMetadata && keepMetadata) {
            return { height, 0 };
        }
        return { 0, height };
    } else if (fitTo == ResizeMode::LONG_EDGE || fitTo == ResizeMode::SHORT_EDGE) {
        // TODO Refactor this section
        if ((fitTo == ResizeMode::LONG_EDGE && originalWidth >= originalHeight) || (fitTo == ResizeMode::SHORT_EDGE && originalWidth <= originalHeight)) {
            if (doNotEnlarge && originalWidth <= size) {
                return { originalWidth, originalHeight };
            }
            return { size, 0 };
        } else if ((fitTo == ResizeMode::LONG_EDGE && originalHeight >= originalWidth) || (fitTo == ResizeMode::SHORT_EDGE && originalHeight <= originalWidth)) {
            if (doNotEnlarge && originalHeight <= size) {
                return { originalWidth, originalHeight };
            }
            return { 0, size };
        }
    }

    return { originalWidth, originalHeight };
}

void showFileInNativeFileManager(const QString& filePath, const QString& fallbackDirectory)
{
    if (!QFileInfo::exists(filePath)) {
        QCaesiumMessageBox msgBox;
        msgBox.setText(QIODevice::tr("File not found"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        return;
    }

#if defined(Q_OS_WIN)
    QStringList param;
    param += QLatin1String("/select,");
    param += QDir::toNativeSeparators(filePath);
    if (QProcess::startDetached("explorer.exe", param)) {
        return;
    }
#elif defined(Q_OS_MAC)
    if (QProcess::startDetached("open", QStringList() << filePath << "-R"))
        return;
#endif
    QDesktopServices::openUrl(QUrl::fromLocalFile(fallbackDirectory));
}

void showDirectoryInNativeFileManager(const QString& dirPath)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath));
}

QJsonObject getSystemData()
{
    QSettings settings;
    QJsonObject systemData {
        { "uuid", settings.value("uuid").toString() },
        { "appVersion", QCoreApplication::applicationVersion() },
        { "kernelType", QSysInfo::kernelType() },
        { "kernelVersion", QSysInfo::kernelVersion() },
        { "productType", QSysInfo::productType() },
        { "productVersion", QSysInfo::productVersion() },
        { "cpuArchitecture", QSysInfo::currentCpuArchitecture() },
    };
    return systemData;
}

QJsonObject getCompressionOptionsAsJSON()
{
    QSettings settings;
    settings.beginGroup("compression_options");
    QMap<QString, QVariant> settingsMap;
    QStringList settingsKeys = settings.allKeys();
    QStringListIterator it(settingsKeys);
    while (it.hasNext()) {
        QString currentKey = it.next();
        settingsMap.insert(currentKey, settings.value(currentKey));
    }
    settings.endGroup();
    return QJsonObject::fromVariantMap(settingsMap);
}

QString getCompressionOptionsHash()
{
    QSettings settings;
    QString implodedSettings;
    settings.beginGroup("compression_options");
    QStringList settingsKeys = settings.allKeys();
    QStringListIterator it(settingsKeys);
    while (it.hasNext()) {
        QString currentKey = it.next();
        implodedSettings.append(currentKey + ":" + settings.value(currentKey).toString() + "||");
    }
    settings.endGroup();
    return hashString(implodedSettings, QCryptographicHash::Sha256);
}

QString hashString(const QString& data, QCryptographicHash::Algorithm algorithm)
{
    QCryptographicHash hash = QCryptographicHash(algorithm);
    hash.addData(data.toUtf8());
    return QString::fromUtf8(hash.result().toHex());
}

QStringList getOutputSupportedFormats()
{
    return {
        QIODevice::tr("Same as input"),
        QIODevice::tr("JPG"),
        QIODevice::tr("PNG"),
        QIODevice::tr("WebP"),
    };
}

bool isRotatedByMetadata(QImageReader* reader)
{
    QFlags<QImageIOHandler::Transformation> transformation = reader->transformation();
    return (transformation == QImageIOHandler::TransformationRotate90
        || transformation == QImageIOHandler::TransformationMirrorAndRotate90
        || transformation == QImageIOHandler::TransformationFlipAndRotate90
        || transformation == QImageIOHandler::TransformationRotate270);
}

QSize getSizeWithMetadata(QImageReader* reader)
{
    QSize imageSize = reader->size();
    QSize actualSize(imageSize.width(), imageSize.height());
    // We need to check if the image is rotated by metadata and adjust the values accordingly
    if (isRotatedByMetadata(reader)) {
        actualSize.setWidth(imageSize.height());
        actualSize.setHeight(imageSize.width());
    }

    return actualSize;
}
