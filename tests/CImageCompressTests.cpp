#include "TestImages.h"
using namespace TestImages;
class CImageCompressTests : public QObject {
    Q_OBJECT
private slots:
    void compressProducesDecodableOutput()
    {
        Corpus c; const auto path = makeImage(c.in, "img.jpg", 640, 480); CImage image(path);
        const auto hash = digest(path); const auto size = QFileInfo(path).size();
        QVERIFY(image.compress(c.options())); QCOMPARE(image.getStatus(), CImageStatus::UNCOMPRESSED);
        QCOMPARE(QFileInfo(image.getCompressedFullPath()).canonicalFilePath(), QFileInfo(c.out.filePath("img.jpg")).canonicalFilePath());
        QCOMPARE(QImageReader(image.getCompressedFullPath()).size(), QSize(640, 480));
        QCOMPARE(image.getCompressedSize(), size_t(QFileInfo(image.getCompressedFullPath()).size()));
        QVERIFY(image.getCompressedSize() < image.getOriginalSize());
        QCOMPARE(digest(path), hash); QCOMPARE(QFileInfo(path).size(), size);
        QCOMPARE(files(c.in), QStringList{"img.jpg"}); QCOMPARE(files(c.out), QStringList{"img.jpg"});
    }
    void outputContentIsComplete()
    {
        Corpus c; CImage image(makeImage(c.in, "img.jpg")); QVERIFY(image.compress(c.options()));
        QImage decoded; QVERIFY(decoded.load(image.getCompressedFullPath()));
        QImageReader reader(image.getCompressedFullPath()); QVERIFY(!reader.read().isNull()); QCOMPARE(reader.error(), QImageReader::UnknownError);
    }
    void skipIfBigger_data() { QTest::addColumn<bool>("existing"); QTest::newRow("copyOriginal") << false; QTest::newRow("preserveExisting") << true; }
    void skipIfBigger()
    {
        QFETCH(bool, existing); Corpus c; auto path = makeImage(c.in, "img.jpg", 256, 256, "jpg", 5);
        const auto inputHash = digest(path); const auto inputSize = QFileInfo(path).size();
        const auto out = c.out.filePath("img.jpg");
        if (existing) QVERIFY(QFile::copy(path, out));
        const auto oldTime = QFileInfo(out).lastModified();
        CImage image(path); auto o = c.options(); o.jpegQuality = 100;
        QVERIFY(image.compress(o)); QCOMPARE(image.getStatus(), CImageStatus::WARNING);
        QVERIFY(image.getFormattedStatus().startsWith("Skipped"));
        QVERIFY(QFileInfo::exists(path)); QCOMPARE(digest(path), inputHash); QCOMPARE(QFileInfo(path).size(), inputSize);
        QCOMPARE(digest(out), inputHash); QCOMPARE(QFileInfo(image.getCompressedFullPath()).canonicalFilePath(), QFileInfo(out).canonicalFilePath());
        if (existing) QCOMPARE(QFileInfo(out).lastModified(), oldTime);
        QCOMPARE(files(c.in), QStringList{"img.jpg"}); QCOMPARE(files(c.out), QStringList{"img.jpg"});
    }
    void noSkipWhenAllowed()
    {
        Corpus c; CImage image(makeImage(c.in, "img.jpg", 256, 256, "jpg", 5)); auto o = c.options(); o.jpegQuality = 100; o.skipIfBigger = false;
        image.setStatus(CImageStatus::COMPRESSING); QVERIFY(image.compress(o));
        QCOMPARE(image.getStatus(), CImageStatus::COMPRESSING); // The caller owns success status transitions.
        QVERIFY(image.getCompressedSize() > image.getOriginalSize()); QVERIFY(!QImage(image.getCompressedFullPath()).isNull());
    }
    void keepDatesApplied_data() { QTest::addColumn<bool>("keep"); QTest::newRow("keep") << true; QTest::newRow("fresh") << false; }
    void keepDatesApplied()
    {
        QFETCH(bool, keep); Corpus c; auto path = makeImage(c.in, "img.jpg"); QFile f(path); QVERIFY(f.open(QIODevice::ReadWrite));
        const QDateTime old(QDate(2001, 2, 3), QTime(12, 0), QTimeZone::UTC);
        QVERIFY(f.setFileTime(old, QFileDevice::FileModificationTime)); f.close();
        CImage image(path); auto o = c.options(); o.keepDates = keep; o.datesMap = {false, true, false};
        QVERIFY(image.compress(o)); const auto actual = QFileInfo(image.getCompressedFullPath()).lastModified();
        QVERIFY(qAbs(actual.secsTo(keep ? old : QDateTime::currentDateTimeUtc())) <= (keep ? 2 : 60));
    }
    void sameFolderOverwriteDeleteOriginal()
    {
        Corpus c; auto path = makeImage(c.in, "img.jpg"); CImage image(path); const auto size = QFileInfo(path).size();
        auto o = c.options(); o.sameFolderAsInput = true; o.moveOriginalFile = true; o.moveOriginalFileDestination = 1;
        QVERIFY(image.compress(o)); QVERIFY(QFileInfo(path).size() < size); QVERIFY(!QImage(path).isNull());
        QCOMPARE(files(c.in), QStringList{"img.jpg"}); QCOMPARE(files(c.out).size(), 0);
    }
    void sameFolderWithSuffix()
    {
        Corpus c; auto path = makeImage(c.in, "img.jpg"); const auto hash = digest(path); CImage image(path);
        auto o = c.options(); o.sameFolderAsInput = true; o.suffix = "_c";
        QVERIFY(image.compress(o)); QCOMPARE(digest(path), hash); QVERIFY(!QImage(c.in.filePath("img_c.jpg")).isNull());
        QCOMPARE(files(c.in), (QStringList{"img.jpg", "img_c.jpg"}));
    }
    void keepStructure()
    {
        Corpus c; CImage image(makeImage(c.in, "a/b/img.jpg")); auto o = c.options(); o.keepStructure = true;
        QVERIFY(image.compress(o)); QCOMPARE(files(c.out), QStringList{"a/b/img.jpg"});
        QVERIFY(!QImage(c.out.filePath("a/b/img.jpg")).isNull());
    }
    void convertFormat()
    {
        Corpus c; CImage image(makeImage(c.in, "img.jpg")); auto o = c.options(); o.format = 3;
        QVERIFY(image.compress(o)); QCOMPARE(files(c.out), QStringList{"img.webp"});
        QImageReader reader(c.out.filePath("img.webp")); QCOMPARE(reader.format(), QByteArray("webp")); QVERIFY(!reader.read().isNull());
    }
    void inputFails_data() { QTest::addColumn<bool>("missing"); QTest::newRow("missingInput") << true; QTest::newRow("corruptInput") << false; }
    void inputFails()
    {
        QFETCH(bool, missing); Corpus c; auto path = makeImage(c.in, "img.jpg"); CImage image(path);
        if (missing) QVERIFY(QFile::remove(path)); else makeCorruptImage(c.in, "img.jpg");
        QVERIFY(!image.compress(c.options())); QCOMPARE(image.getStatus(), CImageStatus::UNCOMPRESSED);
        if (missing) { image.setStatus(CImageStatus::ERROR); QVERIFY(!image.getFormattedStatus().isEmpty()); }
        QVERIFY(files(c.out).isEmpty());
        QCOMPARE(files(c.in), missing ? QStringList{} : QStringList{"img.jpg"});
    }
    void unwritableOutputFails()
    {
#ifdef Q_OS_WIN
        QSKIP("Windows directory read-only attributes do not reliably deny file creation");
#else
        Corpus c; CImage image(makeImage(c.in, "img.jpg"));
        const auto old = QFile::permissions(c.out.path()); QVERIFY(QFile::setPermissions(c.out.path(), QFileDevice::ReadOwner | QFileDevice::ExeOwner));
        struct Restore { QString path; QFile::Permissions permissions; ~Restore() { QFile::setPermissions(path, permissions); } } restore{c.out.path(), old};
        QFile probe(c.out.filePath("probe")); if (probe.open(QIODevice::WriteOnly)) QSKIP("Current user can bypass directory permissions");
        QVERIFY(!image.compress(c.options())); QVERIFY(files(c.out).isEmpty());
#endif
    }
    void previewCacheNotConsulted()
    {
        Corpus c; CImage image(makeImage(c.in, "img.jpg")); QVERIFY(image.compress(c.options())); const auto size = image.getCompressedSize();
        QVERIFY(QFile::remove(image.getCompressedFullPath()));
        const QString preview = image.getTemporaryPreviewFullPath(); QVERIFY(QDir().mkpath(QFileInfo(preview).absolutePath()));
        // Restore a pre-existing test-mode cache entry even on assertion failure.
        QFile previous(preview); const bool existed = previous.exists(); QByteArray bytes;
        if (existed) { QVERIFY(previous.open(QIODevice::ReadOnly)); bytes = previous.readAll(); previous.close(); }
        struct Restore { QString path; bool existed; QByteArray bytes; ~Restore() { QFile::remove(path); if (existed) { QFile f(path); if (f.open(QIODevice::WriteOnly)) f.write(bytes); } } } restore{preview, existed, bytes};
        makeCorruptImage(QFileInfo(preview).dir(), QFileInfo(preview).fileName());
        QVERIFY(image.compress(c.options())); QCOMPARE(image.getCompressedSize(), size); QVERIFY(!QImage(image.getCompressedFullPath()).isNull());
        QCOMPARE(files(c.out), QStringList{"img.jpg"});
    }
};
QObject* makeCImageCompressTests() { return new CImageCompressTests; }
#include "CImageCompressTests.moc"
