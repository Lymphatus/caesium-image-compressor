
#ifndef IMPORTERTESTS_H
#define IMPORTERTESTS_H

#include <QtTest>

class ImporterTests : public QObject
{
    Q_OBJECT

public:
    ImporterTests();
    ~ImporterTests() override;

private slots:
    static void initTestCase();
    static void cleanupTestCase();
    static void rootFolderTestCase();

};

#endif //IMPORTERTESTS_H
