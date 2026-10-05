#include "ImporterTests.h"

#include <services/Importer.h>
#include "TestImages.h"

ImporterTests::ImporterTests()
= default;
ImporterTests::~ImporterTests()
= default;

void ImporterTests::initTestCase()
{
    QVERIFY(true);
}

void ImporterTests::cleanupTestCase()
{
    QVERIFY(true);
}

void ImporterTests::rootFolderTestCase()
{
    QStringList folderMap = QStringList() << "/";
    QString rootFolder = Importer::getRootFolder(folderMap);
    QCOMPARE(rootFolder, QDir("/").absolutePath());

    folderMap = QStringList() << "/1" << "/1/1-1";
    rootFolder = Importer::getRootFolder(folderMap);
    QCOMPARE(rootFolder, QDir("/1").absolutePath());

    folderMap = QStringList() << "/1/1-1" << "/1/1-2";
    rootFolder = Importer::getRootFolder(folderMap);
    QCOMPARE(rootFolder, QDir("/1").absolutePath());

    folderMap = QStringList() << "/1/1-1" << "/1/1-2";
    rootFolder = Importer::getRootFolder(folderMap);
    QCOMPARE(rootFolder, QDir("/1").absolutePath());
}
