#ifndef CIMAGETREEITEM_H
#define CIMAGETREEITEM_H

#include <QFuture>
#include <QVariant>
#include <QVector>
#include <memory>

#include "CImage.h"

class CImageTreeItem {
public:
    explicit CImageTreeItem(CImage* cImage, CImageTreeItem* parentItem = nullptr);
    explicit CImageTreeItem(const QVector<QVariant>& data, CImageTreeItem* parentItem = nullptr);
    ~CImageTreeItem();

    friend bool operator==(const CImageTreeItem& c1, const CImageTreeItem& c2);
    friend bool operator!=(const CImageTreeItem& c1, const CImageTreeItem& c2);

    void appendChild(CImageTreeItem* child);
    void removeChildAt(int position);

    CImageTreeItem* child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    CImageTreeItem* parentItem();
    QVector<CImageTreeItem*> children();

    CImage* getCImage() const;
    std::shared_ptr<CImage> sharedImage() const;
    void refreshFromImage();
    void setDisplayedStatus(CImageStatus status);
    CImageStatus displayedStatus() const;
    void setRelativeFolder(const QString& folder);
    const QString& relativeFolder() const;
    void setDisplayName(const QString& name);
    const QString& displayName() const;
    const QString& cachedRichSize() const;
    const QString& cachedRichResolution() const;
    const QString& cachedRatioText() const;
    const QString& cachedInfoText() const;
    const QString& compressedFullPath() const;
    size_t cachedCompressedSize() const;
    double cachedRatio() const;
    QFuture<void> compress(const CompressionOptions& compressionOptions);
    QFuture<void> compressOnlyFailed(const CompressionOptions& compressionOptions);
    void setCompressionCanceled(bool canceled);

    void setData(QStringList data);

private:
    QVector<CImageTreeItem*> m_childItems;
    QVector<QVariant> m_itemData;
    std::shared_ptr<CImage> cImage;
    CImageStatus cachedStatus {};
    QString cachedRelativeFolder;
    QString cachedDisplayName;
    QString richSize;
    QString richResolution;
    QString ratioText;
    QString infoText;
    QString cachedCompressedFullPath;
    size_t compressedSizeSnapshot = 0;
    double ratioSnapshot = 0;
    CImageTreeItem* m_parentItem;
    bool compressionCanceled = false;

    QFuture<void> performCompression(const CompressionOptions& compressionOptions, bool onlyFailed = false);
};

#endif // CIMAGETREEITEM_H
