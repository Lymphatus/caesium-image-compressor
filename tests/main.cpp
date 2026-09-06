#include <QApplication>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <cstdio>
#include <QtTest>
#include "ImporterTests.h"
QObject* makeCImageTreeModelTests();
QObject* makeSortProxyTests();
QObject* makeCImageCompressTests();
QObject* makeLoggerTests();
#ifdef CAESIUM_TESTS_NEW_API
QObject* makeUtilsTests();
QObject* makeImporterBuildImagesTests();
QObject* makeNewApiModelTests();
QObject* makeCompressionFlowTests();
QObject* makeStressTests();
#endif
int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("SaeraSoft");
    QCoreApplication::setApplicationName("Caesium Image Compressor Tests");
    QStandardPaths::setTestModeEnabled(true);
    int status = 0;
    QTemporaryDir reports;
    if (!reports.isValid()) return 1;
    auto ASSERT_TEST = [&status, argc, argv, &reports](QObject* obj) {
        // Each qExec owns its logger. Keep stdout open across all test classes.
        const QString reportPath = reports.filePath(QString::fromLatin1(obj->metaObject()->className()) + ".txt");
        QStringList arguments{QString::fromLocal8Bit(argv[0])};
        for (int i = 1; i < argc; ++i) {
            if (QString::fromLocal8Bit(argv[i]) == "-o" && i + 1 < argc) { ++i; continue; }
            arguments << QString::fromLocal8Bit(argv[i]);
        }
        arguments << "-o" << reportPath + ",txt";
        status |= QTest::qExec(obj, arguments);
        QFile report(reportPath);
        if (report.open(QIODevice::ReadOnly)) {
            const QByteArray output = report.readAll();
            std::fwrite(output.constData(), 1, size_t(output.size()), stdout);
            std::fflush(stdout);
        } else status |= 1;
        delete obj;
    };
    ASSERT_TEST(new ImporterTests());
    ASSERT_TEST(makeCImageTreeModelTests());
    ASSERT_TEST(makeSortProxyTests());
    ASSERT_TEST(makeCImageCompressTests());
    ASSERT_TEST(makeLoggerTests());
#ifdef CAESIUM_TESTS_NEW_API
    ASSERT_TEST(makeUtilsTests());
    ASSERT_TEST(makeImporterBuildImagesTests());
    ASSERT_TEST(makeNewApiModelTests());
    ASSERT_TEST(makeCompressionFlowTests());
    ASSERT_TEST(makeStressTests());
#endif
    return status;
}
