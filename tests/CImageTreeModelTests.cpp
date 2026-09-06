#include "TestImages.h"
using namespace TestImages;
class CImageTreeModelTests : public QObject {
    Q_OBJECT
private slots:
    void appendEmitsOnce()
    {
        Corpus c; CImageTreeModel m;
        QSignalSpy about(&m, &QAbstractItemModel::rowsAboutToBeInserted), inserted(&m, &QAbstractItemModel::rowsInserted);
        QSignalSpy changed(&m, &CImageTreeModel::itemsChanged), reset(&m, &QAbstractItemModel::modelReset);
        m.appendItems(makeImages(c.in, 200), c.in.path());
        QCOMPARE(about.size(), 1); QCOMPARE(inserted.size(), 1); QCOMPARE(changed.size(), 1); QCOMPARE(reset.size(), 0);
        QVERIFY(!inserted[0][0].value<QModelIndex>().isValid());
        QCOMPARE(inserted[0][1].toInt(), 0); QCOMPARE(inserted[0][2].toInt(), 199); QCOMPARE(m.rowCount(), 200);
    }
    void appendTwiceAppendsAtEnd()
    {
        Corpus c; CImageTreeModel m; m.appendItems(makeImages(c.in, 200), c.in.path());
        QSignalSpy inserted(&m, &QAbstractItemModel::rowsInserted);
        m.appendItems(makeImages(QDir(c.in.filePath("second")), 100), c.in.path());
        QCOMPARE(inserted.size(), 1); QCOMPARE(inserted[0][1].toInt(), 200); QCOMPARE(inserted[0][2].toInt(), 299);
        QCOMPARE(m.rowCount(), 300);
    }
    void displayData()
    {
        Corpus c; CImageTreeModel m;
        auto nested = new CImage(makeImage(c.in, "sub/dir/img.jpg"));
        auto root = new CImage(makeImage(c.in, "root.jpg"));
        m.appendItems({nested, root}, c.in.path());
        const QString name = m.data(m.index(0, 0), Qt::DisplayRole).toString();
        QVERIFY(name.endsWith("img.jpg")); QVERIFY(name.contains("sub/dir/</span>"));
        QVERIFY(m.data(m.index(1, 0), Qt::DisplayRole).toString().contains("></span>root.jpg"));
        QCOMPARE(m.data(m.index(0, 1), Qt::DisplayRole).toString(), nested->getRichFormattedSize());
        QCOMPARE(m.data(m.index(0, 2), Qt::DisplayRole).toString(), QString("256x256"));
        QCOMPARE(m.data(m.index(0, 3), Qt::DisplayRole).toString(), nested->getRichFormattedSavedRatio());
        QCOMPARE(m.data(m.index(0, 4), Qt::DisplayRole).toString(), QString());
        const auto icon = m.data(m.index(0, 0), Qt::DecorationRole);
        QVERIFY(icon.isValid());
        QVERIFY(!qvariant_cast<QPixmap>(icon).isNull() || !qvariant_cast<QIcon>(icon).isNull());
    }
    void finalDisplayData_data()
    {
        QTest::addColumn<int>("status");
        QTest::newRow("compressed") << int(CImageStatus::COMPRESSED);
        QTest::newRow("warning") << int(CImageStatus::WARNING);
        QTest::newRow("error") << int(CImageStatus::ERROR);
    }
    void finalDisplayData()
    {
        QFETCH(int, status);
        const auto expected = CImageStatus(status);
        Corpus c;
        const auto path = makeImage(c.in, "img.jpg", 256, 256, "jpg", expected == CImageStatus::WARNING ? 5 : 100);
        auto image = std::make_unique<CImage>(path);
        auto options = c.options();
        if (expected == CImageStatus::WARNING) options.jpegQuality = 100;
        if (expected == CImageStatus::ERROR) makeCorruptImage(c.in, "img.jpg");
        QCOMPARE(image->compress(options), expected != CImageStatus::ERROR);
        image->setStatus(expected);
        const auto originalSize = image->getOriginalSize();
        const auto compressedSize = image->getCompressedSize();
        CImageTreeModel model;
        model.appendItems({image.release()}, c.in.path());
        const auto info = model.data(model.index(0, INFO_COLUMN), Qt::DisplayRole).toString();
        if (expected == CImageStatus::COMPRESSED) {
            QVERIFY(info.isEmpty());
            const auto size = model.data(model.index(0, SIZE_COLUMN), Qt::DisplayRole).toString();
            QVERIFY(size.contains("<small><s>" + toHumanSize(originalSize) + "</s></small>"));
            QVERIFY(size.endsWith(toHumanSize(compressedSize)));
            QVERIFY(model.data(model.index(0, RATIO_COLUMN), Qt::DisplayRole).toString().contains('%'));
        } else if (expected == CImageStatus::WARNING) QVERIFY(info.startsWith("Skipped"));
        else QVERIFY(!info.isEmpty());
        const auto icon = model.data(model.index(0, NAME_COLUMN), Qt::DecorationRole);
        QVERIFY(!qvariant_cast<QPixmap>(icon).isNull() || !qvariant_cast<QIcon>(icon).isNull());
    }
    void containsAfterRemoveRows()
    {
        Corpus c; CImageTreeModel m; auto images = makeImages(c.in, 3);
        CImage a(images[0]->getFullPath()), b(images[1]->getFullPath());
        m.appendItems(images, c.in.path()); QVERIFY(m.contains(&b));
        QVERIFY(m.removeRows(1, 1)); QVERIFY(!m.contains(&b)); QVERIFY(m.contains(&a)); QCOMPARE(m.rowCount(), 2);
    }
    void originalItemsSizeMatches()
    {
        Corpus c; CImageTreeModel m; auto images = makeImages(c.in, 5);
        double size = 0; for (auto image : images) size += QFileInfo(image->getFullPath()).size();
        m.appendItems(images, c.in.path()); QCOMPARE(m.originalItemsSize(), size);
    }
};
QObject* makeCImageTreeModelTests() { return new CImageTreeModelTests; }
#include "CImageTreeModelTests.moc"
