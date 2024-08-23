#ifndef CIMAGETREEITEM_H
#define CIMAGETREEITEM_H

#include <QFuture>
#include <QVariant>
#include <QVector>

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
    QFuture<void> compress(const CompressionOptions& compressionOptions);
    QFuture<void> compressOnlyFailed(const CompressionOptions& compressionOptions);
    void setCompressionCanceled(bool canceled);

    void setData(QStringList data);

private:
    QVector<CImageTreeItem*> m_childItems;
    QVector<QVariant> m_itemData;
    CImage* cImage;
    CImageTreeItem* m_parentItem;
    bool compressionCanceled = false;

    QFuture<void> performCompression(const CompressionOptions& compressionOptions, bool onlyFailed = false);
};

#endif // CIMAGETREEITEM_H
