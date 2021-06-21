#include <QtTest>

// add necessary includes here

class CsTests : public QObject
{
    Q_OBJECT

public:
    CsTests();
    ~CsTests() override;

private slots:
    void initTestCase();
    void cleanupTestCase();
    void test_case1();

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

void CsTests::test_case1()
{

}

QTEST_APPLESS_MAIN(CsTests)

#include "CsTests.moc"
