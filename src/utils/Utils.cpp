#include "Utils.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <math.h>

QString toHumanSize(size_t size)
{
    //Check if size is 0 to avoid crashes
    if (size == 0) {
        return "0 bytes";
    }

    QStringList unit;
    unit << "bytes"
         << "Kb"
         << "Mb"
         << "Gb"
         << "Tb";
    //Index of the array containing the correct unit
    double order = floor(log2(size) / 10);

    //We should never handle files over 1k Tb, but...
    if (order > 4) {
        qWarning() << "Woah, that's huge!";
        order = 4;
    }

    return QString::number(size / (pow(1024, order)), 'f', 2) + ' ' + unit[(int)order];
}

//TODO Another thread?
QStringList scanDirectory(QString directory)
{
    QStringList inputFilterList = { "*.jpg", "*.jpeg", "*.png" };
    QStringList fileList = {};
    //Collecting all files in folder
    if (QDir(directory).exists()) {
        QDirIterator it(directory,
            inputFilterList,
            QDir::AllEntries,
            QDirIterator::Subdirectories);

        while (it.hasNext()) {
            it.next();
            fileList.append(it.filePath());
        }
    }

    return fileList;
}

cs_image_pars getCompressionParametersFromLevel(int level, bool lossless, bool keepMetadata)
{
    cs_image_pars pars = initialize_parameters();
    pars.jpeg.exif_copy = keepMetadata;
    switch (level) {
    case 0:
        pars.jpeg.quality = 50;
        pars.png.iterations = 1;
        pars.png.iterations_large = 1;
        pars.png.lossy_8 = true;
        pars.png.transparent = true;
        break;
    case 2:
        pars.jpeg.quality = 60;
        pars.png.iterations = 2;
        pars.png.iterations_large = 1;
        pars.png.lossy_8 = true;
        pars.png.transparent = true;
        break;
    case 3:
        pars.jpeg.quality = 70;
        pars.png.iterations = 3;
        pars.png.iterations_large = 1;
        pars.png.lossy_8 = true;
        pars.png.transparent = true;
        break;
    default:
    case 4:
        pars.jpeg.quality = 80;
        pars.png.iterations = 4;
        pars.png.iterations_large = 2;
        pars.png.lossy_8 = true;
        pars.png.transparent = true;
        break;
    case 5:
        pars.jpeg.quality = 100;
        pars.png.iterations = 8;
        pars.png.iterations_large = 4;
        pars.png.lossy_8 = false;
        pars.png.transparent = false;
        break;
    }

    if (lossless) {
        pars.jpeg.quality = 0;
    }
    return pars;
}

QString getRootFolder(QMap<QString, int> folderMap)
{
    QMapIterator<QString, int> it = QMapIterator<QString, int>(folderMap);
    QString rootFolderPath = folderMap.firstKey();
    while (it.hasNext()) {
        QString newFolderPath = it.next().key();
        QStringList splittedNewFolder = newFolderPath.split(QDir::separator());
        QStringList splittedRootFolder = rootFolderPath.split(QDir::separator());
        QStringList splittedCommonPath;

        for (int i = 0; i < std::min(splittedNewFolder.count(), splittedRootFolder.count()); i++) {
            if (QString::compare(splittedNewFolder.at(i), splittedRootFolder.at(i)) != 0) {
                if (i == 0) {
                    rootFolderPath = QDir::rootPath();
                } else {
                    rootFolderPath = QDir(QDir::rootPath() + splittedCommonPath.join(QDir::separator())).absolutePath();
                }
                break;
            }
            splittedCommonPath.append(splittedNewFolder.at(i));
        }
        rootFolderPath = QDir(QDir::rootPath() + splittedCommonPath.join(QDir::separator())).absolutePath();
    }

    return rootFolderPath;
}

QImage cResize(QImage image, int fitTo, int width, int height, int size, bool doNotEnlarge)
{
    int originalWidth = image.width();
    int originalHeight = image.height();

    if (fitTo == ResizeMode::DIMENSIONS) {
        int outputWidth = width;
        int outputHeight = height;
        if (doNotEnlarge && (outputWidth >= originalWidth || outputHeight >= originalHeight)) {
            return image;
        }
        image = image.scaled(outputWidth, outputHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    } else if (fitTo == ResizeMode::PERCENTAGE) {
        int outputWidthPerc = width;
        int outputHeightPerc = height;

        if (doNotEnlarge && (outputWidthPerc >= 100 || outputHeightPerc >= 100)) {
            return image;
        }

        int outputWidth = (int)round((float)originalWidth * (float)outputWidthPerc / 100);
        int outputHeight = (int)round((float)originalHeight * (float)outputHeightPerc / 100);
        image = image.scaled(outputWidth, outputHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    } else if (fitTo == ResizeMode::LONG_EDGE || fitTo == ResizeMode::SHORT_EDGE) {
        if ((fitTo == ResizeMode::LONG_EDGE && originalWidth >= originalHeight) || (fitTo == ResizeMode::SHORT_EDGE && originalWidth <= originalHeight)) {
            image = image.scaledToWidth(size, Qt::SmoothTransformation);
        } else if ((fitTo == ResizeMode::LONG_EDGE && originalHeight >= originalWidth) || (fitTo == ResizeMode::SHORT_EDGE && originalHeight <= originalWidth)) {
            image = image.scaledToHeight(size, Qt::SmoothTransformation);
        }
    }

    return image;
}
