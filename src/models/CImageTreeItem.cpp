#include "CImageTreeItem.h"

#include <QtConcurrent>

CImageTreeItem::CImageTreeItem(CImage* cImage, CImageTreeItem* parent)
    : m_parentItem(parent)
{
    QStringList columnStrings = {
        cImage->getFileName(),
        cImage->getFormattedSize(),
        cImage->getResolution(),
        cImage->getFormattedSavedRatio()
    };

    this->setData(columnStrings);
    this->cImage = cImage;
}

CImageTreeItem::CImageTreeItem(const QVector<QVariant>& data, CImageTreeItem* parent)
    : m_itemData(data)
    , m_parentItem(parent)
{
    this->cImage = nullptr;
}

CImageTreeItem::~CImageTreeItem()
{
    delete cImage;
    qDeleteAll(m_childItems);
}

bool operator== (const CImageTreeItem &c1, const CImageTreeItem &c2)
{
    return (c1.cImage->getFullPath() == c2.cImage->getFullPath());
}

bool operator!= (const CImageTreeItem &c1, const CImageTreeItem &c2)
{
    return !(c1 == c2);
}

void CImageTreeItem::appendChild(CImageTreeItem* item)
{
    m_childItems.append(item);
}

void CImageTreeItem::removeChildAt(int position)
{
    m_childItems.removeAt(position);
}

CImageTreeItem* CImageTreeItem::child(int row)
{
    if (row < 0 || row >= m_childItems.size())
        return nullptr;
    return m_childItems.at(row);
}

int CImageTreeItem::childCount() const
{
    return m_childItems.count();
}

int CImageTreeItem::row() const
{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<CImageTreeItem*>(this));

    return 0;
}

int CImageTreeItem::columnCount() const
{
    return m_itemData.count();
}

QVariant CImageTreeItem::data(int column) const
{
    if (column < 0 || column >= m_itemData.size())
        return QVariant();
    return m_itemData.at(column);
}

CImageTreeItem* CImageTreeItem::parentItem()
{
    return m_parentItem;
}

QVector<CImageTreeItem*> CImageTreeItem::children()
{
    return m_childItems;
}

CImage *CImageTreeItem::getCImage() const
{
    return cImage;
}

QFuture<void> CImageTreeItem::compress(CompressionOptions compressionOptions)
{
    return QtConcurrent::map(m_childItems, [compressionOptions](CImageTreeItem* item) {
        item->getCImage()->setStatus(CImageStatus::COMPRESSING);
        bool compressionResult = item->cImage->compress(compressionOptions);
        item->getCImage()->setStatus(compressionResult ? CImageStatus::COMPRESSED : CImageStatus::ERROR);
    });
}

void CImageTreeItem::setData(int column, QString data)
{
    this->m_itemData.replace(column, QVariant(data));
}

void CImageTreeItem::setData(QStringList data)
{
    QVector<QVariant> columnData;
    columnData.reserve(data.count());
    for (const QString& columnString : data) {
        columnData << columnString;
    }
    this->m_itemData = columnData;
}
