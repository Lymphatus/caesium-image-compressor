#include <QtTest>
#include <services/Importer.h>

class CsTests : public QObject
{
    Q_OBJECT

public:
    CsTests();
    ~CsTests() override;

private slots:
    void initTestCase();
    void cleanupTestCase();
};

CsTests::CsTests()
= default;

CsTests::~CsTests()
= default;

void CsTests::initTestCase()
{

}

void CsTests::cleanupTestCase()
{

}

QTEST_APPLESS_MAIN(CsTests)

#include "CsTests.moc"
