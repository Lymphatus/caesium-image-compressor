#ifndef IMPORTER_H
#define IMPORTER_H
#include <QFileInfo>
#include <QString>
#include <QList>
#include <atomic>
#include <functional>

class CImage;
typedef struct ImportResult {
    QList<CImage*> images;
    int skippedCount = 0;
    bool canceled = false;
} ImportResult;

typedef struct SkipBySizeFilter {
    bool enabled;
    int unit;
    int condition;
    int size;
} SkipBySize;

typedef struct FilenameRegexFilter {
    bool enabled;
    QString filter;
} FilenameRegexFilter;

typedef struct ImportFilters {
    SkipBySizeFilter skipBySizeFilter {};
    FilenameRegexFilter filenameRegexFilter {};
} ImportFilters;

enum ImportFromArgsMethod {
    IMPORT_ONLY = 0,
    IMPORT_AND_COMPRESS = 1,
};

class Importer {

public:
    static QString getRootFolder(QList<QString> folderMap);
    static QStringList scanDirectory(const QString& directory, bool subfolders);
    static QStringList scanDirectory(const QString& directory, bool subfolders, const ImportFilters& importFilters);
    static QStringList scanList(const QStringList& filesAndFolders, bool subfolders);
    static QStringList scanList(const QStringList& filesAndFolders, bool subfolders, const std::atomic_bool* cancelFlag);
    static ImportResult buildImages(const QStringList& files, const std::atomic_bool* cancelFlag = nullptr, const std::function<void(int)>& progress = {});
    static bool passesFilters(const QFileInfo& fileInfo, const ImportFilters& importFilters);
private:
    static QStringList scanDirectory(bool hasFilters, const QString& directory, bool subfolders, const ImportFilters& importFilters);
    static QStringList scanDirectory(bool hasFilters, const QString& directory, bool subfolders, const ImportFilters& importFilters, const std::atomic_bool* cancelFlag);
};

#endif // IMPORTER_H
