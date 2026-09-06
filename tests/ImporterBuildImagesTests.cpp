#include "TestImages.h"
#include <atomic>
#include <functional>
#include <services/Importer.h>
using namespace TestImages;
namespace {
struct OwnedImport {
    ImportResult result;
    explicit OwnedImport(ImportResult value) : result(std::move(value)) {}
    ~OwnedImport() { qDeleteAll(result.images); }
};
}
class ImporterBuildImagesTests : public QObject {
    Q_OBJECT
private slots:
    void buildImagesMetadata()
    {
        Corpus c;
        QStringList paths{makeImage(c.in,"a.jpg",320,200), makeImage(c.in,"b.png",100,300,"png"), makeImage(c.in,"c.webp",64,64,"webp")};
        OwnedImport owned(Importer::buildImages(paths)); const auto& r = owned.result;
        QCOMPARE(r.images.size(), 3); QCOMPARE(r.skippedCount, 0); QVERIFY(!r.canceled);
        const QStringList sizes{"320x200", "100x300", "64x64"}, formats{"jpg", "png", "webp"};
        for (int i = 0; i < 3; ++i) {
            const QFileInfo info(paths[i]); auto image = r.images[i];
            QCOMPARE(image->getFileName(), info.fileName()); QCOMPARE(image->getFullPath(), info.canonicalFilePath());
            QCOMPARE(image->getDirectory(), QFileInfo(info.canonicalFilePath()).path());
            QCOMPARE(image->getOriginalSize(), size_t(info.size())); QCOMPARE(image->getResolution(), sizes[i]);
            QCOMPARE(image->getFormat(), formats[i]); QCOMPARE(image->getStatus(), CImageStatus::UNCOMPRESSED);
        }
    }
    void buildImagesSkipsUnsupported()
    {
        Corpus c; QFile text(c.in.filePath("text.txt")); QVERIFY(text.open(QIODevice::WriteOnly)); text.write("not an image"); text.close();
        QVERIFY(text.rename(c.in.filePath("text.png")));
        OwnedImport owned(Importer::buildImages({makeImage(c.in,"a.jpg"),makeCorruptImage(c.in),text.fileName(),makeEmptyFile(c.in)}));
        QCOMPARE(owned.result.images.size(), 1); QCOMPARE(owned.result.skippedCount, 3); QVERIFY(!owned.result.canceled);
    }
    void buildImagesKeepsDuplicates()
    {
        Corpus c; const auto path = makeImage(c.in,"a.jpg"); OwnedImport owned(Importer::buildImages({path,path,path}));
        QCOMPARE(owned.result.images.size(), 3); QCOMPARE(owned.result.skippedCount, 0);
        for (auto image : owned.result.images) QCOMPARE(image->getFullPath(), QFileInfo(path).canonicalFilePath());
        QVERIFY(owned.result.images[0] != owned.result.images[1]); QVERIFY(owned.result.images[1] != owned.result.images[2]); QVERIFY(owned.result.images[0] != owned.result.images[2]);
    }
    void buildImagesCancelFlag()
    {
        Corpus c; QStringList paths; for (int i=0;i<200;++i) paths << makeImage(c.in,QString("i%1.jpg").arg(i));
        std::atomic_bool cancel{false}; QList<int> progress;
        OwnedImport owned(Importer::buildImages(paths, &cancel, [&](int done) { progress << done; if (done == 50) cancel.store(true); }));
        QVERIFY(owned.result.canceled); QVERIFY(owned.result.images.size() >= 50); QVERIFY(owned.result.images.size() <= 51);
        QVERIFY(progress.contains(50)); for (int done : progress) QVERIFY(done <= 51);
    }
    void buildImagesProgressMonotonic()
    {
        Corpus c; QStringList paths{makeImage(c.in,"a.jpg"),makeCorruptImage(c.in),makeImage(c.in,"b.jpg"),makeEmptyFile(c.in)};
        QList<int> progress; OwnedImport owned(Importer::buildImages(paths,nullptr,[&](int done) { progress << done; }));
        QVERIFY(!progress.isEmpty()); QCOMPARE(progress.last(), paths.size());
        for(int i=1;i<progress.size();++i) QVERIFY(progress[i] > progress[i-1]);
        QCOMPARE(owned.result.skippedCount, 2);
    }
    void buildImagesOwnership()
    {
        Corpus c; auto result = Importer::buildImages({makeImage(c.in,"a.jpg"),makeImage(c.in,"b.jpg")});
        QCOMPARE(result.images.size(), 2); for (auto image : result.images) delete image;
    }
};
QObject* makeImporterBuildImagesTests() { return new ImporterBuildImagesTests; }
#include "ImporterBuildImagesTests.moc"
