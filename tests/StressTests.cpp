#include "TestImages.h"
#include <services/Importer.h>
#include <QCollator>
#include <QTimer>

using namespace TestImages;
quint64 privateBytes();
namespace {
struct OwnedImages {
    ImportResult result;
    ~OwnedImages() { qDeleteAll(result.images); }
    QList<CImage*> take() { QList<CImage*> images; images.swap(result.images); return images; }
};
}

class StressTests : public QObject {
    Q_OBJECT
private slots:
    void largeBatch_data()
    {
        QTest::addColumn<QString>("operation");
        for (const char* name : {"importStress", "appendAndSortStress", "secondImportDedupe",
                                "compressStress", "overwriteStress", "cancelStress", "clearStress", "memoryCeiling"})
            QTest::newRow(name) << QString::fromLatin1(name);
    }
    void largeBatch()
    {
        if (skipStress()) QSKIP("CAESIUM_SKIP_STRESS=1");
        QFETCH(QString, operation);
        const int n = stressN();
        const double scale = n / 1000.0;
        Corpus c;
        QStringList validNames;
        QHash<QString, QByteArray> hashes;
        // N is the total valid count: 98% JPEG, 1% PNG and 1% WebP.
        for (int i = 0; i < n; ++i) {
            const char* format = i < n / 100 ? "png" : i < 2 * (n / 100) ? "webp" : "jpg";
            const QString name = QString("img%1.%2").arg(n - i).arg(format);
            const QString path = makeImage(c.in, name, 256, 256, format, 100);
            validNames << name;
            hashes.insert(name, digest(path));
        }
        for (int i = 0; i < 3; ++i) makeCorruptImage(c.in, QString("bad%1.jpg").arg(i));
        makeEmptyFile(c.in);
        const QStringList inputFiles = files(c.in);
        QElapsedTimer timer;
        timer.start();
        const auto scanned = Importer::scanDirectory(c.in.path(), true);
        const auto scanMs = timer.elapsed();
        timer.restart();
        OwnedImages imported{Importer::buildImages(scanned)};
        const auto buildMs = timer.elapsed();
        qInfo().noquote() << operation << "N:" << n << "scan ms:" << scanMs << "build ms:" << buildMs;
        QCOMPARE(imported.result.images.size(), n);
        QCOMPARE(imported.result.skippedCount, 4);
        QVERIFY(!imported.result.canceled);
        if (operation == "importStress") {
            QVERIFY(scanMs < 5000 * scale);
            QVERIFY(buildMs < 30000 * scale);
            return;
        }
        CImageTreeModel model;
        CImageSortFilterProxyModel proxy;
        proxy.setSourceModel(&model);
        proxy.setDynamicSortFilter(true);
        proxy.sort(NAME_COLUMN);
        QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);
        timer.restart();
        model.appendItems(imported.take(), c.in.path());
        const auto appendMs = timer.elapsed();
        if (operation == "appendAndSortStress") {
            qInfo() << "append/sort ms:" << appendMs;
            QCOMPARE(inserted.size(), 1);
            QCOMPARE(inserted[0][1].toInt(), 0);
            QCOMPARE(inserted[0][2].toInt(), n - 1);
            QCollator collator;
            collator.setNumericMode(true);
            collator.setCaseSensitivity(Qt::CaseSensitive);
            std::stable_sort(validNames.begin(), validNames.end(), [&](const QString& a, const QString& b) {
                return collator.compare(a, b) < 0;
            });
            QCOMPARE(proxyNames(proxy, model), validNames);
            QVERIFY(appendMs < 2000 * scale);
            return;
        }
        if (operation == "secondImportDedupe") {
            OwnedImages second{Importer::buildImages(scanned)};
            QList<CImage*> remainder;
            timer.restart();
            for (auto image : second.result.images) if (!model.contains(image)) remainder << image;
            const auto containsMs = timer.elapsed();
            qInfo() << "second import contains ms:" << containsMs << "new rows:" << remainder.size();
            // Retain ownership of duplicates, transfer only genuinely new objects.
            for (auto image : remainder) second.result.images.removeOne(image);
            inserted.clear();
            if (!remainder.isEmpty()) model.appendItems(remainder, c.in.path());
            QCOMPARE(remainder.size(), 0);
            QCOMPARE(inserted.size(), 0);
            QCOMPARE(model.rowCount(), n);
            QVERIFY(containsMs < 300 * scale);
            return;
        }

        QObject context;
        QList<int> started, finished;
        // Pool is destroyed before objects referenced by its workers and queued observers.
        QThreadPool pool;
        pool.setMaxThreadCount(defaultMaxThreads());
        const bool cancel = operation == "cancelStress";
        const int cancelAt = qMax(1, n / 20);
        connect(&model, &CImageTreeModel::itemCompressionStarted, &context,
                [&](int row) { started << row; }, Qt::QueuedConnection);
        connect(&model, &CImageTreeModel::itemCompressionFinished, &context, [&](int row) {
            finished << row;
            if (cancel && finished.size() == cancelAt) model.cancelCompression();
        }, Qt::QueuedConnection);
        auto options = c.options();
        const bool overwrite = operation == "overwriteStress";
        if (overwrite) {
            options.sameFolderAsInput = true;
            options.moveOriginalFile = true;
            options.moveOriginalFileDestination = 1;
        }
        quint64 peak = privateBytes();
        QTimer memoryTimer;
        connect(&memoryTimer, &QTimer::timeout, &context, [&] { peak = qMax(peak, privateBytes()); });
        memoryTimer.start(250);
        QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);
        timer.restart();
        auto future = model.compress(&pool, options);
        const int timeout = cancel ? 60000 : qMax(1000, int(600000 * scale));
        QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(), timeout);
        const auto compressMs = timer.elapsed();
        model.flushPendingUpdates();
        bool drained = false;
        QMetaObject::invokeMethod(&context, [&] { drained = true; }, Qt::QueuedConnection);
        QTRY_VERIFY_WITH_TIMEOUT(drained, 5000);
        memoryTimer.stop();
        peak = qMax(peak, privateBytes());
        qInfo().noquote() << operation << "compress ms:" << compressMs << "started:" << started.size()
                         << "finished:" << finished.size() << "dataChanged:" << changed.size() << "peak bytes:" << peak;
        QVERIFY(compressMs < timeout);
        QVERIFY(!model.isCompressing());
        QCOMPARE(started.size(), finished.size());
        const QSet<int> processed(finished.begin(), finished.end());
        QCOMPARE(processed.size(), finished.size());
        QCOMPARE(QSet<int>(started.begin(), started.end()), processed);
        if (cancel) {
            QVERIFY(finished.size() >= cancelAt);
            QVERIFY(finished.size() < n);
        } else {
            QCOMPARE(finished.size(), n);
            // Amendment 4 relaxes the coalescing bound, including this stress workload.
            QVERIFY(changed.size() <= n / 2);
        }
        QStringList expectedOutputs;
        for (int row = 0; row < n; ++row) {
            auto item = model.getRootItem()->child(row);
            auto image = item->getCImage();
            const auto name = image->getFileName();
            const bool done = processed.contains(row);
            const auto status = image->getStatus();
            // skipIfBigger is on: a synthetic PNG/WebP may legitimately end as WARNING (kept original), never ERROR.
            if (done) QVERIFY2(status == CImageStatus::COMPRESSED || status == CImageStatus::WARNING, qPrintable(name + ": " + image->getFormattedStatus()));
            else QCOMPARE(status, CImageStatus::UNCOMPRESSED);
            QCOMPARE(item->displayedStatus(), status);
            if (done) {
                const auto output = overwrite ? c.in.filePath(name) : c.out.filePath(name);
                QImageReader reader(output);
                const QImage decoded = reader.read();
                QVERIFY2(!decoded.isNull(), qPrintable(output + ": " + reader.errorString()));
                QCOMPARE(decoded.size(), QSize(256, 256));
                if (overwrite) {
                    if (status == CImageStatus::COMPRESSED) QVERIFY(digest(output) != hashes.value(name));
                    else QCOMPARE(digest(output), hashes.value(name));
                } else expectedOutputs << name;
            } else QVERIFY(!QFileInfo::exists(c.out.filePath(name)));
            if (!overwrite) QCOMPARE(digest(c.in.filePath(name)), hashes.value(name));
        }
        expectedOutputs.sort();
        QCOMPARE(files(c.out), expectedOutputs);
        QCOMPARE(files(c.in), inputFiles);
        if (operation == "clearStress") {
            QList<int> rows;
            for (int i = 0; i < n; ++i) rows << i;
            QSignalSpy removed(&model, &QAbstractItemModel::rowsRemoved), items(&model, &CImageTreeModel::itemsChanged);
            timer.restart();
            const bool removedAll = model.removeItems(rows);
            const auto clearMs = timer.elapsed();
            qInfo() << "clear compressed rows ms:" << clearMs;
            QVERIFY(removedAll);
            QCOMPARE(model.rowCount(), 0);
            QCOMPARE(removed.size(), 1);
            QCOMPARE(removed[0][1].toInt(), 0);
            QCOMPARE(removed[0][2].toInt(), n - 1);
            QCOMPARE(items.size(), 1);
            QVERIFY(clearMs < 500 * scale);
        }
        if (operation == "memoryCeiling") {
            if (!peak) QSKIP("Process memory counter unavailable on this platform");
            if (qEnvironmentVariableIntValue("CAESIUM_ENFORCE_MEMORY") == 1)
                QVERIFY(peak < quint64(1.5 * 1024 * 1024 * 1024));
        }
    }
};
QObject* makeStressTests() { return new StressTests; }
#include "StressTests.moc"
