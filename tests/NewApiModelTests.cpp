#include "TestImages.h"
using namespace TestImages;
class NewApiModelTests : public QObject {
    Q_OBJECT
private slots:
    void containsIsConstantTime()
    {
        if (skipStress()) QSKIP("CAESIUM_SKIP_STRESS=1");
        Corpus c; CImageTreeModel m; auto images = makeImages(c.in,5000);
        QList<std::shared_ptr<CImage>> probes; for(auto image:images) probes << std::make_shared<CImage>(image->getFullPath());
        m.appendItems(images,c.in.path());
        bool all = true; QElapsedTimer timer; timer.start(); for(const auto& probe:probes) all = m.contains(probe.get()) && all;
        const auto ms = timer.elapsed(); qInfo() << "contains 5000 ms:" << ms; QVERIFY(all); QVERIFY(ms < 300);
        CImage absent(makeImage(c.in,"absent.jpg")); QVERIFY(!m.contains(&absent));
    }
    void containsAfterRemove()
    {
        Corpus c; CImageTreeModel m; auto images=makeImages(c.in,3); CImage a(images[0]->getFullPath()), b(images[1]->getFullPath());
        m.appendItems(images,c.in.path()); QVERIFY(m.removeItems({1})); QVERIFY(!m.contains(&b)); QVERIFY(m.contains(&a));
    }
    void modelContainsRefCount()
    {
        Corpus c; CImageTreeModel m; const auto path=makeImage(c.in,"a.jpg"); CImage probe(path);
        m.appendItems({new CImage(path)},c.in.path()); m.appendItems({new CImage(path)},c.in.path());
        QVERIFY(m.removeItems({1})); QVERIFY(m.contains(&probe)); QVERIFY(m.removeItems({0})); QVERIFY(!m.contains(&probe));
    }
    void removeItemsRanges()
    {
        Corpus c; CImageTreeModel m; m.appendItems(makeImages(c.in,20),c.in.path());
        QSignalSpy removed(&m,&QAbstractItemModel::rowsRemoved), changed(&m,&CImageTreeModel::itemsChanged);
        QVERIFY(m.removeItems({3,4,5,10,12,12,99,-1})); QCOMPARE(m.rowCount(),15); QCOMPARE(removed.size(),3); QCOMPARE(changed.size(),1);
        const QList<QPair<int,int>> ranges{{12,12},{10,10},{3,5}};
        for(int i=0;i<3;++i) { QVERIFY(!removed[i][0].value<QModelIndex>().isValid()); QCOMPARE(removed[i][1].toInt(),ranges[i].first); QCOMPARE(removed[i][2].toInt(),ranges[i].second); }
        int row=0; for(int i=0;i<20;++i) if(i!=3 && i!=4 && i!=5 && i!=10 && i!=12) QCOMPARE(m.getRootItem()->child(row++)->getCImage()->getFileName(),QString("img%1.jpg").arg(i));
    }
    void removeItemsAll()
    {
        if(skipStress()) QSKIP("CAESIUM_SKIP_STRESS=1");
        Corpus c; CImageTreeModel m; m.appendItems(makeImages(c.in,5000),c.in.path()); QList<int> rows; for(int i=0;i<5000;++i) rows << i;
        QSignalSpy removed(&m,&QAbstractItemModel::rowsRemoved), changed(&m,&CImageTreeModel::itemsChanged);
        QElapsedTimer timer; timer.start(); const bool ok=m.removeItems(rows); const auto ms=timer.elapsed(); qInfo() << "remove 5000 ms:" << ms;
        QVERIFY(ok); QVERIFY(ms<500); QCOMPARE(m.rowCount(),0); QCOMPARE(removed.size(),1); QCOMPARE(changed.size(),1);
        QCOMPARE(removed[0][1].toInt(),0); QCOMPARE(removed[0][2].toInt(),4999);
    }
    void removeItemsEmpty()
    {
        CImageTreeModel m; QSignalSpy removed(&m,&QAbstractItemModel::rowsRemoved), changed(&m,&CImageTreeModel::itemsChanged), reset(&m,&QAbstractItemModel::modelReset);
        QVERIFY(!m.removeItems({})); QCOMPARE(removed.size(),0); QCOMPARE(changed.size(),0); QCOMPARE(reset.size(),0);
    }
    void cacheAccessorsAndSharedOwnership()
    {
        Corpus c; auto image=new CImage(makeImage(c.in,"img.jpg")); std::shared_ptr<CImage> retained;
        { CImageTreeItem item(image); retained=item.sharedImage(); QCOMPARE(retained.get(),image);
          item.setRelativeFolder("sub/"); QCOMPARE(item.relativeFolder(),QString("sub/"));
          item.setDisplayedStatus(CImageStatus::COMPRESSING); QCOMPARE(item.displayedStatus(),CImageStatus::COMPRESSING);
          QCOMPARE(item.cachedInfoText(),QIODevice::tr("Compressing...")); QCOMPARE(image->getStatus(),CImageStatus::UNCOMPRESSED);
          QVERIFY(image->compress(c.options())); image->setStatus(CImageStatus::COMPRESSED); item.refreshFromImage();
          QCOMPARE(item.displayedStatus(),CImageStatus::COMPRESSED); QCOMPARE(item.compressedFullPath(),image->getCompressedFullPath());
          QCOMPARE(item.cachedCompressedSize(),image->getCompressedSize()); QCOMPARE(item.cachedRatio(),image->getRatio());
          QCOMPARE(item.cachedRichSize(),image->getRichFormattedSize());
          QCOMPARE(item.cachedRatioText(),image->getRichFormattedSavedRatio());
        }
        QCOMPARE(retained->getFileName(),QString("img.jpg"));
    }
    void baseFolderChangeRefreshesExistingRows()
    {
        Corpus c; CImageTreeModel m; m.appendItems({new CImage(makeImage(c.in,"sub/a.jpg"))},c.in.filePath("sub"));
        QSignalSpy changed(&m,&QAbstractItemModel::dataChanged);
        m.appendItems({new CImage(makeImage(c.in,"b.jpg"))},c.in.path()); QCOMPARE(changed.size(),1);
        QCOMPARE(changed[0][0].value<QModelIndex>(),m.index(0,0)); QCOMPARE(changed[0][1].value<QModelIndex>(),m.index(0,0));
        QVERIFY(m.data(m.index(0,0),Qt::DisplayRole).toString().contains("sub/</span>a.jpg"));
    }
};
QObject* makeNewApiModelTests() { return new NewApiModelTests; }
#include "NewApiModelTests.moc"
