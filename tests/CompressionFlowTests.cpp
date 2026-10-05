#include "BatchFixture.h"
#include <QSemaphore>
#include <QtConcurrent>
using namespace TestImages;
class CompressionFlowTests : public QObject {
    Q_OBJECT
private slots:
    void signalsPerRow()
    {
        BatchFixture b(50); auto future=b.model.compress(&b.pool,b.corpus.options());
        QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(),30000); QTRY_COMPARE_WITH_TIMEOUT(b.finished.size(),50,5000); QTRY_COMPARE_WITH_TIMEOUT(b.started.size(),50,5000);
        QSet<int> starts, ends;
        for(const auto& event:b.events) { QVERIFY(event.first>=0 && event.first<50); if(event.second) { QVERIFY(!starts.contains(event.first)); starts.insert(event.first); } else { QVERIFY(starts.contains(event.first)); QVERIFY(!ends.contains(event.first)); ends.insert(event.first); } }
        QCOMPARE(starts.size(),50); QCOMPARE(ends,starts);
    }
    void statusesAfterBatchAndOnlyFailedRecompresses()
    {
        BatchFixture b(53); for(int i=50;i<53;++i) makeCorruptImage(b.corpus.in,QString("img%1.jpg").arg(i));
        auto future=b.model.compress(&b.pool,b.corpus.options()); QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(),30000);
        QTRY_COMPARE_WITH_TIMEOUT(b.finished.size(),53,5000); QTRY_COMPARE_WITH_TIMEOUT(b.started.size(),53,5000); b.model.flushPendingUpdates();
        for(int i=0;i<53;++i) {
            const auto expected=i<50?CImageStatus::COMPRESSED:CImageStatus::ERROR;
            auto item=b.model.getRootItem()->child(i); QCOMPARE(item->getCImage()->getStatus(),expected); QCOMPARE(item->displayedStatus(),expected);
            QVERIFY(b.model.data(b.model.index(i,0),Qt::DecorationRole).isValid()); QCOMPARE(b.model.data(b.model.index(i,4),Qt::DisplayRole).toString().isEmpty(),i<50);
        }
        QVERIFY(b.model.compressedItemsSize()<b.model.originalItemsSize());
        for(int i=50;i<53;++i) makeImage(b.corpus.in,QString("img%1.jpg").arg(i));
        b.started.clear(); b.finished.clear();
        future=b.model.compress(&b.pool,b.corpus.options(),true); QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(),30000);
        QTRY_COMPARE_WITH_TIMEOUT(b.started.size(),3,5000); QTRY_COMPARE_WITH_TIMEOUT(b.finished.size(),3,5000); b.model.flushPendingUpdates();
        QCOMPARE(QSet<int>(b.started.begin(),b.started.end()),(QSet<int>{50,51,52}));
        for(int i=0;i<53;++i) { QCOMPARE(b.model.getRootItem()->child(i)->displayedStatus(),CImageStatus::COMPRESSED); QCOMPARE(b.model.getRootItem()->child(i)->getCImage()->getStatus(),CImageStatus::COMPRESSED); }
    }
    void coalescedDataChanged()
    {
        if(skipStress()) QSKIP("CAESIUM_SKIP_STRESS=1");
        const int n=stressN(); BatchFixture b(n,4,32,32); CImageSortFilterProxyModel proxy; proxy.setSourceModel(&b.model); proxy.setDynamicSortFilter(true); proxy.sort(NAME_COLUMN);
        QSignalSpy changed(&b.model,&QAbstractItemModel::dataChanged), layout(&proxy,&QAbstractItemModel::layoutChanged);
        auto future=b.model.compress(&b.pool,b.corpus.options()); QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(),600000); b.model.flushPendingUpdates();
        qInfo()<<"coalesced dataChanged:"<<changed.size()<<"rows:"<<n<<"proxy layouts:"<<layout.size(); QVERIFY(changed.size()<=n/2); QVERIFY(layout.size()<=changed.size()+1);
        QSet<int> covered;
        for(const auto& args:changed) { auto first=args[0].value<QModelIndex>(),last=args[1].value<QModelIndex>(); QCOMPARE(first.column(),0); QCOMPARE(last.column(),4); QVERIFY(first.row()>=0); QVERIFY(last.row()<n); QVERIFY(first.row()<=last.row()); for(int r=first.row();r<=last.row();++r) covered.insert(r); }
        QCOMPARE(covered.size(),n);
    }
    void flushIsSynchronous()
    {
        BatchFixture b(20); QSignalSpy changed(&b.model,&QAbstractItemModel::dataChanged); b.model.flushPendingUpdates(); QCOMPARE(changed.size(),0);
        // Hold workers until the GUI thread can poll the future without delivering queued signals.
        // A bounded semaphore handshake avoids relying on event-loop flush timing.
        QSemaphore release;
        b.pool.setMaxThreadCount(1);
        auto gate=QtConcurrent::run(&b.pool,[&] { release.acquire(); });
        auto future=b.model.compress(&b.pool,b.corpus.options()); release.release();
        QElapsedTimer deadline; deadline.start();
        while(!future.isFinished() && deadline.elapsed()<30000) QThread::yieldCurrentThread();
        QVERIFY(future.isFinished()); QCOMPARE(changed.size(),0);
        b.model.flushPendingUpdates(); QVERIFY(!changed.isEmpty());
        for(int r=0;r<20;++r) QCOMPARE(b.model.getRootItem()->child(r)->displayedStatus(),CImageStatus::COMPRESSED);
        changed.clear(); b.model.flushPendingUpdates(); QCOMPARE(changed.size(),0); QVERIFY(!b.model.isCompressing());
    }
    void cancelStopsScheduling()
    {
        BatchFixture b(400,1); const auto cancelConnection = QObject::connect(&b.model,&CImageTreeModel::itemCompressionFinished,&b.context,[&](int) { if(b.finished.size()==10) b.model.cancelCompression(); },Qt::QueuedConnection);
        auto future=b.model.compress(&b.pool,b.corpus.options()); QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(),30000);
        // Delivery of a queued sentinel establishes that all worker observer events have drained.
        bool drained=false; QMetaObject::invokeMethod(&b.context,[&] {drained=true;},Qt::QueuedConnection); QTRY_VERIFY_WITH_TIMEOUT(drained,5000);
        b.model.flushPendingUpdates(); QVERIFY(b.finished.size()>=10); QVERIFY(b.finished.size()<400); QVERIFY(!b.model.isCompressing());
        QSet<int> started(b.started.begin(),b.started.end()); QCOMPARE(started.size(),b.finished.size());
        for(int i=0;i<400;++i) if(!started.contains(i)) { QCOMPARE(b.model.getRootItem()->child(i)->displayedStatus(),CImageStatus::UNCOMPRESSED); QVERIFY(!QFileInfo::exists(b.corpus.out.filePath(QString("img%1.jpg").arg(i)))); }
        QObject::disconnect(cancelConnection);
        b.started.clear(); b.finished.clear();
        future=b.model.compress(&b.pool,b.corpus.options()); QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(),60000); QTRY_COMPARE_WITH_TIMEOUT(b.finished.size(),400,5000); b.model.flushPendingUpdates();
        for(int i=0;i<400;++i) QCOMPARE(b.model.getRootItem()->child(i)->displayedStatus(),CImageStatus::COMPRESSED);
    }
    void noRowsIsNoOp()
    {
        BatchFixture b(0); auto future=b.model.compress(&b.pool,b.corpus.options()); QVERIFY(future.isFinished()); b.model.flushPendingUpdates();
        QCoreApplication::processEvents(); QVERIFY(b.started.isEmpty()); QVERIFY(b.finished.isEmpty()); QVERIFY(!b.model.isCompressing());
    }
    void modelSafeToUseDuringBatch()
    {
        BatchFixture b(100); int reads=0; QTimer timer;
        QObject::connect(&timer,&QTimer::timeout,&b.context,[&] { for(int r=0;r<100;++r) for(int col=0;col<5;++col) { b.model.data(b.model.index(r,col),Qt::DisplayRole); b.model.data(b.model.index(r,col),Qt::DecorationRole); ++reads; } });
        timer.start(10); auto future=b.model.compress(&b.pool,b.corpus.options()); QTRY_VERIFY_WITH_TIMEOUT(future.isFinished(),30000); timer.stop(); b.model.flushPendingUpdates();
        QVERIFY(reads>0); for(int r=0;r<100;++r) QCOMPARE(b.model.getRootItem()->child(r)->displayedStatus(),CImageStatus::COMPRESSED);
    }
};
QObject* makeCompressionFlowTests() { return new CompressionFlowTests; }
#include "CompressionFlowTests.moc"
