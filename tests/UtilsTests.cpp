#include "TestImages.h"
#include <QThread>
class UtilsTests : public QObject {
    Q_OBJECT
private slots:
    void defaultMaxThreadsBounds()
    {
        const int value = defaultMaxThreads(); QVERIFY(value >= 1); QVERIFY(value <= 8);
        const int ideal = QThread::idealThreadCount();
        if (ideal > 0) QVERIFY(value <= ideal); else QCOMPARE(value, 1);
    }
};
QObject* makeUtilsTests() { return new UtilsTests; }
#include "UtilsTests.moc"
