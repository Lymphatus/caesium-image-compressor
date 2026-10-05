#include "TestImages.h"
#include <utils/Logger.h>
#include <QtConcurrent>
using namespace TestImages;
namespace {
struct LogCapture {
    QtMessageHandler previous;
    LogCapture() : previous(qInstallMessageHandler(Logger::messageHandler)) {}
    ~LogCapture() { qInstallMessageHandler(previous); Logger::closeLogFile(); }
};
QStringList logLines()
{
    QFile f(Logger::getLogFilePath());
    if (!f.open(QIODevice::ReadOnly)) return {};
    return QString::fromUtf8(f.readAll()).split('\n', Qt::SkipEmptyParts);
}
}
class LoggerTests : public QObject {
    Q_OBJECT
private slots:
    void init()
    {
        Logger::closeLogFile();
        const auto path = Logger::getLogFilePath();
        if (QFile::exists(path)) QVERIFY(QFile::remove(path));
    }
    void throughput()
    {
        if (skipStress()) QSKIP("CAESIUM_SKIP_STRESS=1");
        qint64 ms;
        { LogCapture capture; QElapsedTimer timer; timer.start();
          for (int i = 0; i < 10000; ++i) qWarning().noquote() << QString("throughput %1").arg(i);
          ms = timer.elapsed(); }
        qInfo() << "Logger 10000 messages ms:" << ms; QVERIFY(ms < 3000);
        const auto lines = logLines(); QCOMPARE(lines.size(), 10000);
        const QRegularExpression pattern(R"(^\[\d{4}-\d{2}-\d{2} [^\]]+\]\[W\] throughput \d+\r?$)");
        for (int i = 0; i < lines.size(); ++i) { QVERIFY2(pattern.match(lines[i]).hasMatch(), qPrintable(lines[i])); QVERIFY(lines[i].trimmed().endsWith(QString("throughput %1").arg(i))); }
    }
    void concurrentWritersDoNotInterleave()
    {
        QThreadPool pool; pool.setMaxThreadCount(8); QList<int> workers{0,1,2,3,4,5,6,7};
        { LogCapture capture;
          auto future = QtConcurrent::map(&pool, workers, [](int worker) {
              for (int i = 0; i < 500; ++i) qWarning().noquote() << QString("worker %1 sequence %2").arg(worker).arg(i);
          });
          QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), 30000);
        }
        const auto lines = logLines(); QCOMPARE(lines.size(), 4000); QSet<QString> messages;
        const QRegularExpression pattern(R"(^\[\d{4}-\d{2}-\d{2} [^\]]+\]\[W\] (worker [0-7] sequence \d+)\r?$)");
        for (const auto& line : lines) { const auto match = pattern.match(line); QVERIFY2(match.hasMatch(), qPrintable(line)); messages.insert(match.captured(1)); }
        QCOMPARE(messages.size(), 4000);
        for (int w = 0; w < 8; ++w) for (int i = 0; i < 500; ++i) QVERIFY(messages.contains(QString("worker %1 sequence %2").arg(w).arg(i)));
    }
    void closeAndReopen()
    {
        { LogCapture capture; qWarning("first"); Logger::closeLogFile(); QCOMPARE(logLines().size(), 1); qWarning("second"); }
        const auto lines = logLines(); QCOMPARE(lines.size(), 2); QVERIFY(lines[0].trimmed().endsWith("first")); QVERIFY(lines[1].trimmed().endsWith("second"));
    }
    void logDirectoryCreated()
    {
        const QString path = QDir(Logger::getLogDir()).absolutePath();
        // Deletion is confined to this application's explicit test-mode directory.
        QVERIFY(path.contains("Caesium Image Compressor Tests"));
        QVERIFY(QStandardPaths::isTestModeEnabled());
        QDir dir(path); if (dir.exists()) QVERIFY(dir.removeRecursively());
        { LogCapture capture; qWarning("create directory"); }
        QVERIFY(QDir(path).exists()); QCOMPARE(logLines().size(), 1);
    }
};
QObject* makeLoggerTests() { return new LoggerTests; }
#include "LoggerTests.moc"
