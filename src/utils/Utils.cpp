#include "Utils.h"

#include <QDir>
#include <QDirIterator>
#include <cmath>
#include <QJsonObject>
#include <QImageReader>

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

//TODO Another thread?
QStringList scanDirectory(QString directory, bool subfolders)
{
    QStringList inputFilterList = { "*.jpg", "*.jpeg", "*.png", "*.webp" };
    QStringList fileList = {};
    auto iteratorFlags = subfolders ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
    //Collecting all files in folder
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

QString getRootFolder(QMap<QString, int> folderMap)
{
    QMapIterator<QString, int> it = QMapIterator<QString, int>(folderMap);
    QString rootFolderPath = folderMap.firstKey();
    while (it.hasNext()) {
        QString newFolderPath = it.next().key();
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

std::tuple<unsigned int, unsigned int> cResize(QSize originalSize, int fitTo, int width, int height, int size, bool doNotEnlarge)
{
    int originalWidth = originalSize.width();
    int originalHeight = originalSize.height();

    if (fitTo == ResizeMode::DIMENSIONS) {
        int outputWidth = width;
        int outputHeight = height;
        if (doNotEnlarge && (outputWidth >= originalWidth || outputHeight >= originalHeight)) {
            return {originalWidth, originalHeight};
        }
        return {outputWidth, outputHeight};
    } else if (fitTo == ResizeMode::PERCENTAGE) {
        int outputWidthPerc = width;
        int outputHeightPerc = height;

        if (doNotEnlarge && (outputWidthPerc >= 100 || outputHeightPerc >= 100)) {
            return {originalWidth, originalHeight};
        }

        int outputWidth = (int)round((double)originalWidth * (double)outputWidthPerc / 100);
        int outputHeight = (int)round((double)originalHeight * (double)outputHeightPerc / 100);
        return {outputWidth, outputHeight};
    } else if (fitTo == ResizeMode::LONG_EDGE || fitTo == ResizeMode::SHORT_EDGE) {
        //TODO Refactor this section
        if ((fitTo == ResizeMode::LONG_EDGE && originalWidth >= originalHeight) || (fitTo == ResizeMode::SHORT_EDGE && originalWidth <= originalHeight)) {
            if (doNotEnlarge && originalWidth <= size) {
                return {originalWidth, originalHeight};
            }
            return {size, 0};
        } else if ((fitTo == ResizeMode::LONG_EDGE && originalHeight >= originalWidth) || (fitTo == ResizeMode::SHORT_EDGE && originalHeight <= originalWidth)) {
            if (doNotEnlarge && originalHeight <= size) {
                return {originalWidth, originalHeight};
            }
            return {0, size};
        }
    }

    return {originalWidth, originalHeight};
}
