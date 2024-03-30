#ifndef IMPORTER_H
#define IMPORTER_H
#include <QFileInfo>
#include <QString>

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
    static bool passesFilters(const QFileInfo& fileInfo, const ImportFilters& importFilters);
private:
    static QStringList scanDirectory(bool hasFilters, const QString& directory, bool subfolders, const ImportFilters& importFilters);
};

#endif // IMPORTER_H
