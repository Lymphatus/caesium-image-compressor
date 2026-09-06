#include "TestImages.h"
#include <QCollator>
using namespace TestImages;
class SortProxyTests : public QObject {
    Q_OBJECT
private slots:
    void naturalOrderByName_data() { QTest::addColumn<bool>("descending"); QTest::newRow("ascending") << false; QTest::newRow("descending") << true; }
    void naturalOrderByName()
    {
        QFETCH(bool, descending);
        Corpus c; CImageTreeModel m; CImageSortFilterProxyModel p;
        p.setSourceModel(&m); p.setDynamicSortFilter(true); p.sort(NAME_COLUMN, descending ? Qt::DescendingOrder : Qt::AscendingOrder);
        QStringList expected{"img10.jpg", "img2.jpg", "img1.jpg", "Img3.jpg", "a-1.jpg"};
        QList<CImage*> images; for (const auto& name : expected) images << new CImage(makeImage(c.in, name));
        m.appendItems(images, c.in.path());
        QCollator collator; collator.setNumericMode(true); collator.setCaseSensitivity(Qt::CaseSensitive);
        std::stable_sort(expected.begin(), expected.end(), [&](const QString& a, const QString& b) { return collator.compare(a, b) < 0; });
        QCOMPARE(expected.first(), QString("a-1.jpg"));
        QVERIFY(expected.indexOf("img1.jpg") < expected.indexOf("img2.jpg"));
        QVERIFY(expected.indexOf("img2.jpg") < expected.indexOf("img10.jpg"));
        if (descending) std::reverse(expected.begin(), expected.end());
        QCOMPARE(proxyNames(p, m), expected);
    }
    void folderGrouping()
    {
        Corpus c; CImageTreeModel m; CImageSortFilterProxyModel p; p.setSourceModel(&m); p.setDynamicSortFilter(true); p.sort(NAME_COLUMN);
        QList<CImage*> images; for (auto name : {"z.jpg", "a/b.jpg", "a/a.jpg", "b/a.jpg", "a.jpg"}) images << new CImage(makeImage(c.in, name));
        m.appendItems(images, c.in.path());
        QStringList actual;
        for (int i = 0; i < p.rowCount(); ++i) actual << c.in.relativeFilePath(m.getRootItem()->child(p.mapToSource(p.index(i, 0)).row())->getCImage()->getFullPath());
        QCOMPARE(actual, (QStringList{"a.jpg", "z.jpg", "a/a.jpg", "a/b.jpg", "b/a.jpg"}));
    }
    void numericColumns_data()
    {
        QTest::addColumn<int>("column");
        QTest::newRow("size") << int(SIZE_COLUMN); QTest::newRow("resolution") << int(RESOLUTION_COLUMN); QTest::newRow("ratio") << int(RATIO_COLUMN);
    }
    void numericColumns()
    {
        QFETCH(int, column);
        Corpus c; CImageTreeModel m; CImageSortFilterProxyModel p; p.setSourceModel(&m); p.setDynamicSortFilter(true);
        QList<CImage*> images;
        for (int n : {256, 64, 128, 128}) images << new CImage(makeImage(c.in, QString("f%1.jpg").arg(images.size()), n, n));
        // Include distinct nonzero ratios and a tie without mutating model-owned images.
        if (column == RATIO_COLUMN) for (auto image : images) { auto o = c.options(); QVERIFY(image->compress(o)); }
        QList<CImage*> expected = images;
        auto key = [column](CImage* i) -> double { return column == SIZE_COLUMN ? i->getOriginalSize() : column == RESOLUTION_COLUMN ? i->getTotalPixels() : i->getRatio(); };
        std::stable_sort(expected.begin(), expected.end(), [&](CImage* a, CImage* b) { return key(a) < key(b); });
        m.appendItems(images, c.in.path()); p.sort(column);
        QStringList names; for (auto i : expected) names << i->getFileName();
        QCOMPARE(proxyNames(p, m), names);
    }
    void noRelayoutForNonSortColumns()
    {
        Corpus c; CImageTreeModel m; CImageSortFilterProxyModel p; p.setSourceModel(&m); p.setDynamicSortFilter(true); p.sort(NAME_COLUMN);
        m.appendItems(makeImages(c.in, 100), c.in.path());
        QSignalSpy layout(&p, &QAbstractItemModel::layoutChanged), changed(&p, &QAbstractItemModel::dataChanged);
        for (int r = 0; r < 100; ++r) emit m.dataChanged(m.index(r, 1), m.index(r, 4));
        QCOMPARE(layout.size(), 0); QCOMPARE(changed.size(), 100);
    }
    void noRelayoutWithDynamicSortOff()
    {
        Corpus c; CImageTreeModel m; CImageSortFilterProxyModel p; p.setSourceModel(&m); p.setDynamicSortFilter(true); p.sort(NAME_COLUMN);
        m.appendItems(makeImages(c.in, 12), c.in.path()); p.setDynamicSortFilter(false);
        QSignalSpy layout(&p, &QAbstractItemModel::layoutChanged);
        emit m.dataChanged(m.index(0, 0), m.index(11, 4)); QCOMPARE(layout.size(), 0);
        p.setDynamicSortFilter(true); QCOMPARE(layout.size(), 1);
        QStringList expected; for (int i = 0; i < 12; ++i) expected << QString("img%1.jpg").arg(i);
        QCOMPARE(proxyNames(p, m), expected);
    }
    void sortCost()
    {
        if (skipStress()) QSKIP("CAESIUM_SKIP_STRESS=1");
        Corpus c; CImageTreeModel m; CImageSortFilterProxyModel p; p.setSourceModel(&m); p.setDynamicSortFilter(true); p.sort(NAME_COLUMN);
        auto images = makeImages(c.in, 5000);
        QElapsedTimer timer; timer.start(); m.appendItems(images, c.in.path()); auto ms = timer.elapsed();
        qInfo() << "sortCost ms:" << ms; QVERIFY(ms < 1500);
    }
};
QObject* makeSortProxyTests() { return new SortProxyTests; }
#include "SortProxyTests.moc"
