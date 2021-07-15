#include "Utils.h"

#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <cmath>
#include <exiv2/exiv2.hpp>

QString toHumanSize(size_t size)
{
    if (size == 0) {
        return "0 bytes";
    }

    QStringList unit;
    unit << "bytes"
         << "Kb"
         << "Mb"
         << "Gb"
         << "Tb";
    double order = floor(log2(size) / 10);

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

//cs_image_pars getCompressionParametersFromLevel(int level, bool lossless, bool keepMetadata)
//{
//    cs_image_pars pars = initialize_parameters();
//    pars.jpeg.exif_copy = keepMetadata;
//    switch (level) {
//    case 0:
//        pars.jpeg.quality = 50;
//        pars.png.iterations = 1;
//        pars.png.iterations_large = 1;
//        pars.png.lossy_8 = true;
//        pars.png.transparent = true;
//        break;
//    case 2:
//        pars.jpeg.quality = 60;
//        pars.png.iterations = 2;
//        pars.png.iterations_large = 1;
//        pars.png.lossy_8 = true;
//        pars.png.transparent = true;
//        break;
//    case 3:
//        pars.jpeg.quality = 70;
//        pars.png.iterations = 3;
//        pars.png.iterations_large = 1;
//        pars.png.lossy_8 = true;
//        pars.png.transparent = true;
//        break;
//    default:
//    case 4:
//        pars.jpeg.quality = 80;
//        pars.png.iterations = 4;
//        pars.png.iterations_large = 2;
//        pars.png.lossy_8 = true;
//        pars.png.transparent = true;
//        break;
//    case 5:
//        pars.jpeg.quality = 100;
//        pars.png.iterations = 8;
//        pars.png.iterations_large = 4;
//        pars.png.lossy_8 = false;
//        pars.png.transparent = false;
//        break;
//    }
//
//    if (lossless) {
//        pars.jpeg.quality = 0;
//    }
//    return pars;
//}

QString getRootFolder(QMap<QString, int> folderMap)
{
    QMapIterator<QString, int> it = QMapIterator<QString, int>(folderMap);
    QString rootFolderPath = folderMap.firstKey();
    while (it.hasNext()) {
        QString newFolderPath = it.next().key();
        QStringList splitNewFolder = QDir::toNativeSeparators(newFolderPath).split(QDir::separator());
        QStringList splitRootFolder = QDir::toNativeSeparators(rootFolderPath).split(QDir::separator());
        QStringList splitCommonPath;

        for (int i = 0; i < std::min(splitNewFolder.count(), splitRootFolder.count()); i++) {
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

        int outputWidth = (int)round((double)originalWidth * (double)outputWidthPerc / 100);
        int outputHeight = (int)round((double)originalHeight * (double)outputHeightPerc / 100);
        image = image.scaled(outputWidth, outputHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    } else if (fitTo == ResizeMode::LONG_EDGE || fitTo == ResizeMode::SHORT_EDGE) {
        //TODO Refactor this section
        if ((fitTo == ResizeMode::LONG_EDGE && originalWidth >= originalHeight) || (fitTo == ResizeMode::SHORT_EDGE && originalWidth <= originalHeight)) {
            if (doNotEnlarge && originalWidth <= size) {
                return image;
            }
            image = image.scaledToWidth(size, Qt::SmoothTransformation);
        } else if ((fitTo == ResizeMode::LONG_EDGE && originalHeight >= originalWidth) || (fitTo == ResizeMode::SHORT_EDGE && originalHeight <= originalWidth)) {
            if (doNotEnlarge && originalHeight <= size) {
                return image;
            }
            image = image.scaledToHeight(size, Qt::SmoothTransformation);
        }
    }

    return image;
}

bool copyMetadata(const char* input, const char* output)
{
    try {
        Exiv2::XmpParser::initialize();
        ::atexit(Exiv2::XmpParser::terminate);
#ifdef EXV_ENABLE_BMFF
        Exiv2::enableBMFF();
#endif

        auto readImg = Exiv2::ImageFactory::open(input);
        readImg->readMetadata();

        auto writeImg = Exiv2::ImageFactory::open(output);
        writeImg->readMetadata();
        writeImg->setIptcData(readImg->iptcData());
        writeImg->setExifData(readImg->exifData());
        writeImg->setComment(readImg->comment());
        writeImg->setXmpData(readImg->xmpData());

        try {
            writeImg->writeMetadata();
        } catch (const Exiv2::AnyError&) {
            return false;
        }

        return true;
    } catch (Exiv2::AnyError& e) {
        return false;
    }
}
