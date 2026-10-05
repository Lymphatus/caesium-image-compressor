#pragma once
#include <QtTest>
#include <QDirIterator>
#include <QPainter>
#include <QRandomGenerator>
#include <QTemporaryDir>
#include <models/CImageTreeModel.h>
#include <models/CImageSortFilterProxyModel.h>
#include <memory>
#include <algorithm>

namespace TestImages {
inline QString makeImage(const QDir& dir, const QString& name, int w = 256,
                         int h = 256, const char* format = "jpg", int quality = 100)
{
    const QString path = dir.filePath(name);
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) qFatal("Cannot create fixture directory");
    QImage image(w, h, QImage::Format_RGB32);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            image.setPixel(x, y, qRgb(x * 255 / w, y * 255 / h, (x + y) * 255 / (w + h)));
    QRandomGenerator random(42);
    QPainter painter(&image);
    for (int i = 0; i < 20; ++i)
        painter.fillRect(random.bounded(w), random.bounded(h), random.bounded(w / 4 + 1) + 1,
                         random.bounded(h / 4 + 1) + 1, QColor::fromRgb(random.generate() | 0xff000000));
    painter.end();
    if (!image.save(path, format, quality)) qFatal("Cannot save synthetic image");
    return path;
}
inline QString makeCorruptImage(const QDir& dir, const QString& name = "corrupt.jpg")
{
    const QString path = dir.filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) qFatal("Cannot corrupt fixture");
    QRandomGenerator random(42);
    QByteArray bytes(512, '\0');
    for (char& byte : bytes) byte = char(random.bounded(256));
    if (file.write(bytes) != bytes.size()) qFatal("Cannot write corrupt fixture");
    return path;
}
inline QString makeEmptyFile(const QDir& dir, const QString& name = "empty.jpg")
{
    QFile file(dir.filePath(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) qFatal("Cannot create empty fixture");
    return file.fileName();
}
inline QList<CImage*> makeImages(const QDir& dir, int n, int w = 256, int h = 256)
{
    QList<CImage*> images;
    for (int i = 0; i < n; ++i) images << new CImage(makeImage(dir, QString("img%1.jpg").arg(i), w, h));
    return images;
}
inline CompressionOptions defaultOptions(const QString& outputDir)
{
    CompressionOptions o{};
    o.outputPath = outputDir;
    o.basePath = QDir(outputDir).filePath("../in");
    o.jpegQuality = 80; o.pngQuality = 80; o.pngOptimizationLevel = 3;
    o.webpQuality = 60; o.keepMetadata = true; o.skipIfBigger = true;
    o.compressionMode = QUALITY; o.maxOutputSize = {MAX_OUTPUT_BYTES, 0};
    return o;
}
inline int stressN()
{
    bool ok = false;
    int n = qEnvironmentVariableIntValue("CAESIUM_STRESS_N", &ok);
    return ok && n > 0 ? n : 1000;
}
inline bool skipStress() { return qEnvironmentVariableIntValue("CAESIUM_SKIP_STRESS") == 1; }
inline QByteArray digest(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) qFatal("Cannot hash fixture");
    return QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256);
}
inline QStringList files(const QDir& dir)
{
    QStringList result;
    QDirIterator it(dir.absolutePath(), QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) result << dir.relativeFilePath(it.next());
    result.sort();
    return result;
}
struct Corpus {
    QTemporaryDir temp;
    QDir in, out;
    Corpus() : in(temp.filePath("in")), out(temp.filePath("out"))
    {
        if (!temp.isValid() || !QDir().mkpath(in.path()) || !QDir().mkpath(out.path()))
            qFatal("Cannot create corpus");
    }
    CompressionOptions options() const { auto o = defaultOptions(out.path()); o.basePath = in.path(); return o; }
};
inline QStringList proxyNames(CImageSortFilterProxyModel& proxy, CImageTreeModel& model)
{
    QStringList names;
    for (int r = 0; r < proxy.rowCount(); ++r)
        names << model.getRootItem()->child(proxy.mapToSource(proxy.index(r, 0)).row())->getCImage()->getFileName();
    return names;
}
}
