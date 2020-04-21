#ifndef CIMAGETREEITEM_H
#define CIMAGETREEITEM_H

#include <QVariant>
#include <QVector>
#include <QFuture>

#include "CImage.h"

class CImageTreeItem {
public:
    explicit CImageTreeItem(CImage* cImage, CImageTreeItem* parentItem = nullptr);
    explicit CImageTreeItem(const QVector<QVariant>& data, CImageTreeItem* parentItem = nullptr);
    ~CImageTreeItem();

    friend bool operator== (const CImageTreeItem &c1, const CImageTreeItem &c2);
    friend bool operator!= (const CImageTreeItem &c1, const CImageTreeItem &c2);

    void appendChild(CImageTreeItem* child);
    void removeChildAt(int position);

    CImageTreeItem* child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    CImageTreeItem* parentItem();
    QVector<CImageTreeItem*> children();

    CImage *getCImage() const;
    QFuture<void> compress(CompressionOptions compressionOptions);
    void setData(QStringList data);

private:
    QVector<CImageTreeItem*> m_childItems;
    QVector<QVariant> m_itemData;
    CImage* cImage;
    CImageTreeItem* m_parentItem;
};

#endif // CIMAGETREEITEM_H
