#include "Importer.h"

#include <QDirIterator>
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
    QStringList filesList;
    QStringListIterator it(filesAndFolders);

    while (it.hasNext()) {
        QString path = it.next();
        QFileInfo info = QFileInfo(path);
        if (info.isDir()) {
            filesList.append(scanDirectory(path, subfolders));
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
    QStringList fileList;
    QDirIterator::IteratorFlags flags = subfolders ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags;
    QDirIterator it(directory, {"*.jpg", "*.jpeg", "*.png", "*.webp", "*.tif", "*.tiff"}, QDir::AllEntries, flags);

    while (it.hasNext()) {
        QString filePath = it.next();
        if (hasFilters && !passesFilters(QFileInfo(filePath), importFilters)) {
            continue;
        }
        fileList.append(filePath);
    }

    return fileList;
}
