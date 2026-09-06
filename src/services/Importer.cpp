#include "Importer.h"

#include <QDirIterator>
#include <QDebug>
#include <exceptions/ImageNotSupportedException.h>
#include <exceptions/ImageTooBigException.h>
#include <models/CImage.h>

QString Importer::getRootFolder(QList<QString> folderMap)
{
    if (folderMap.isEmpty()) {
        return QDir::rootPath();
    }
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

bool Importer::passesFilters(const QFileInfo& fileInfo, const ImportFilters& importFilters)
{
    if (importFilters.skipBySizeFilter.enabled) {
        int unit = importFilters.skipBySizeFilter.unit;
        int condition = importFilters.skipBySizeFilter.condition;
        int size = importFilters.skipBySizeFilter.size << (unit * 10);
        size_t imageSize = fileInfo.size();
        if ((condition == 0 && imageSize > size) || (condition == 1 && imageSize == size) || (condition == 2 && imageSize < size)) {
            return false;
        }
    }

    if (importFilters.filenameRegexFilter.enabled) {
        QString filename = fileInfo.fileName();

        QString pattern = importFilters.filenameRegexFilter.filter;
        QRegularExpression regex(pattern);

        if (!regex.match(filename).hasMatch()) {
            return false;
        }
    }

    return true;
}

QStringList Importer::scanList(const QStringList& filesAndFolders, bool subfolders)
{
    return scanList(filesAndFolders, subfolders, nullptr);
}

QStringList Importer::scanList(const QStringList& filesAndFolders, bool subfolders, const std::atomic_bool* cancelFlag)
{
    QStringList filesList;
    QStringListIterator it(filesAndFolders);

    while (it.hasNext()) {
        if (cancelFlag && cancelFlag->load()) {
            break;
        }
        QString path = it.next();
        QFileInfo info = QFileInfo(path);
        if (info.isDir()) {
            filesList.append(scanDirectory(false, path, subfolders, ImportFilters(), cancelFlag));
        } else if (info.isFile()) {
            filesList.append(path);
        }
    }

    return filesList;
}

QStringList Importer::scanDirectory(const QString& directory, bool subfolders)
{
    return scanDirectory(false, directory, subfolders, ImportFilters());
}

QStringList Importer::scanDirectory(const QString& directory, bool subfolders, const ImportFilters& importFilters)
{
    return scanDirectory(true, directory, subfolders, importFilters);
}

QStringList Importer::scanDirectory(bool hasFilters, const QString& directory, bool subfolders, const ImportFilters& importFilters = ImportFilters())
{
    return scanDirectory(hasFilters, directory, subfolders, importFilters, nullptr);
}

QStringList Importer::scanDirectory(bool hasFilters, const QString& directory, bool subfolders, const ImportFilters& importFilters, const std::atomic_bool* cancelFlag)
{
    QStringList fileList;
    QDirIterator::IteratorFlags flags = subfolders ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
    const QStringList nameFilters = { "*.jpg", "*.jpeg", "*.png", "*.webp", "*.tif", "*.tiff" };
    // Visit even nonmatching entries so cancellation also works in folders without images.
    QDirIterator it(directory, cancelFlag ? QStringList() : nameFilters, QDir::AllEntries, flags);

    while (!(cancelFlag && cancelFlag->load()) && it.hasNext()) {
        QString filePath = it.next();
        if (cancelFlag && !QDir::match(nameFilters, it.fileName())) {
            continue;
        }
        if (hasFilters && !passesFilters(QFileInfo(filePath), importFilters)) {
            continue;
        }
        fileList.append(filePath);
    }

    return fileList;
}

ImportResult Importer::buildImages(const QStringList& files, const std::atomic_bool* cancelFlag, const std::function<void(int)>& progress)
{
    ImportResult result;
    for (int i = 0; i < files.size(); ++i) {
        if (cancelFlag && cancelFlag->load()) {
            result.canceled = true;
            break;
        }
        try {
            result.images.append(new CImage(files.at(i)));
        } catch (ImageNotSupportedException& e) {
            ++result.skippedCount;
            qWarning() << files.at(i) << "is not supported. Error:" << e.what();
        } catch (ImageTooBigException& e) {
            ++result.skippedCount;
            qWarning() << files.at(i) << "is too big. Error:" << e.what();
        }
        if (progress) {
            progress(i + 1);
        }
    }
    return result;
}
