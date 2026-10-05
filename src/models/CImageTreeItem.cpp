#include "CImageTreeItem.h"

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
    this->cImage.reset(cImage);
    refreshFromImage();
}

CImageTreeItem::CImageTreeItem(const QVector<QVariant>& data, CImageTreeItem* parent)
    : m_itemData(data)
    , m_parentItem(parent)
{
    this->cImage = nullptr;
}

CImageTreeItem::~CImageTreeItem()
{
    qDeleteAll(m_childItems);
}

bool operator==(const CImageTreeItem& c1, const CImageTreeItem& c2)
{
    return (c1.cImage->getFullPath() == c2.cImage->getFullPath());
}

bool operator!=(const CImageTreeItem& c1, const CImageTreeItem& c2)
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

CImage* CImageTreeItem::getCImage() const
{
    return cImage.get();
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

std::shared_ptr<CImage> CImageTreeItem::sharedImage() const
{
    return cImage;
}

void CImageTreeItem::refreshFromImage()
{
    cachedStatus = cImage->getStatus();
    richSize = cImage->getRichFormattedSize();
    richResolution = cImage->getRichResolution();
    ratioText = cImage->getRichFormattedSavedRatio();
    infoText = cImage->getFormattedStatus();
    cachedCompressedFullPath = cImage->getCompressedFullPath();
    compressedSizeSnapshot = cImage->getCompressedSize();
    ratioSnapshot = cImage->getRatio();
}

void CImageTreeItem::setDisplayedStatus(CImageStatus status)
{
    cachedStatus = status;
    if (status == CImageStatus::COMPRESSING) {
        infoText = QIODevice::tr("Compressing...");
    }
}

CImageStatus CImageTreeItem::displayedStatus() const
{
    return cachedStatus;
}

void CImageTreeItem::setRelativeFolder(const QString& folder)
{
    cachedRelativeFolder = folder;
}

const QString& CImageTreeItem::relativeFolder() const
{
    return cachedRelativeFolder;
}

void CImageTreeItem::setDisplayName(const QString& name)
{
    cachedDisplayName = name;
}

const QString& CImageTreeItem::displayName() const
{
    return cachedDisplayName;
}

const QString& CImageTreeItem::cachedRichSize() const
{
    return richSize;
}

const QString& CImageTreeItem::cachedRichResolution() const
{
    return richResolution;
}

const QString& CImageTreeItem::cachedRatioText() const
{
    return ratioText;
}

const QString& CImageTreeItem::cachedInfoText() const
{
    return infoText;
}

const QString& CImageTreeItem::compressedFullPath() const
{
    return cachedCompressedFullPath;
}

size_t CImageTreeItem::cachedCompressedSize() const
{
    return compressedSizeSnapshot;
}

double CImageTreeItem::cachedRatio() const
{
    return ratioSnapshot;
}
