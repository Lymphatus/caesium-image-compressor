#include "ImporterTests.h"

#include <services/Importer.h>

ImporterTests::ImporterTests()
= default;
ImporterTests::~ImporterTests()
= default;

void ImporterTests::initTestCase()
{
    Q_ASSERT(true);
}

void ImporterTests::cleanupTestCase()
{
    Q_ASSERT(true);
}

void ImporterTests::rootFolderTestCase()
{
    QStringList folderMap = QStringList() << "/";
    QString rootFolder = Importer::getRootFolder(folderMap);
    Q_ASSERT(rootFolder == QString("/"));

    folderMap = QStringList() << "/1" << "/1/1-1";
    rootFolder = Importer::getRootFolder(folderMap);
    Q_ASSERT(rootFolder == QString("/1"));

    folderMap = QStringList() << "/1/1-1" << "/1/1-2";
    rootFolder = Importer::getRootFolder(folderMap);
    Q_ASSERT(rootFolder == QString("/1"));

    folderMap = QStringList() << "/1/1-1" << "/1/1-2";
    rootFolder = Importer::getRootFolder(folderMap);
    Q_ASSERT(rootFolder == QString("/1"));
}