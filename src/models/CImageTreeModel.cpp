#include "CImageTreeModel.h"

#include <QIcon>

CImageTreeModel::CImageTreeModel()
{
    rootItem = new CImageTreeItem({ tr("Name"), tr("Size"), tr("Resolution"), tr("Saved") });
}

CImageTreeModel::~CImageTreeModel()
{
    delete rootItem;
}

QModelIndex CImageTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    CImageTreeItem* parentItem;

    if (!parent.isValid()) {
        parentItem = rootItem;
    } else {
        parentItem = static_cast<CImageTreeItem*>(parent.internalPointer());
    }

    CImageTreeItem* childItem = parentItem->child(row);
    if (childItem) {
        return createIndex(row, column, childItem);
    }
    return QModelIndex();
}

QModelIndex CImageTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    CImageTreeItem* childItem = static_cast<CImageTreeItem*>(index.internalPointer());
    CImageTreeItem* parentItem = childItem->parentItem();

    if (parentItem == rootItem) {

        return QModelIndex();
    }
    return createIndex(parentItem->row(), 0, parentItem);
}

int CImageTreeModel::rowCount(const QModelIndex& parent) const
{
    CImageTreeItem* parentItem;

    if (parent.column() > 0) {
        return 0;
    }

    if (!parent.isValid()) {
        parentItem = rootItem;
    } else {
        parentItem = static_cast<CImageTreeItem*>(parent.internalPointer());
    }

    return parentItem->childCount();
}

int CImageTreeModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return static_cast<CImageTreeItem*>(parent.internalPointer())->columnCount();
    }
    return rootItem->columnCount();
}

bool CImageTreeModel::removeRows(int row, int count, const QModelIndex& parent)
{
    beginRemoveRows(parent, row, row + count - 1);

    for (int i = 0; i < count; i++) {
        this->rootItem->removeChildAt(row);
    }

    endRemoveRows();
    emit itemsChanged();
    return true;
}

void CImageTreeModel::appendItems(QList<CImage*> imageList)
{
    this->setupModelData(imageList, rootItem);
}

void CImageTreeModel::setupModelData(const QList<CImage*> imageList, CImageTreeItem* parent)
{
    QListIterator<CImage*> iterator(imageList);
    this->beginInsertRows(QModelIndex(), this->rowCount(), this->rowCount() + imageList.count() - 1);
    while (iterator.hasNext()) {
        CImage* nextImage = iterator.next();
        auto* cImageTreeItem = new CImageTreeItem(nextImage, parent);
        parent->appendChild(cImageTreeItem);
    }
    endInsertRows();
    emit itemsChanged();
}

void CImageTreeModel::emitDataChanged(int row)
{
    QModelIndex modelIndexStart = this->index(row, 0);
    QModelIndex modelIndexEnd = this->index(row, this->columnCount() - 1);
    emit dataChanged(modelIndexStart, modelIndexEnd);
}

CImageTreeItem* CImageTreeModel::getRootItem() const
{
    return rootItem;
}

bool CImageTreeModel::contains(CImage* cImage)
{
    QVectorIterator<CImageTreeItem*> it(this->rootItem->children());
    while (it.hasNext()) {
        if (*it.next()->getCImage() == *cImage) {
            return true;
        }
    }
    return false;
}

QVariant CImageTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole && role != Qt::DecorationRole) {
        return QVariant();
    }

    CImageTreeItem* item = static_cast<CImageTreeItem*>(index.internalPointer());
    if (role == Qt::DecorationRole && index.column() == CImageColumns::NAME) {
        CImageStatus status = item->getCImage()->getStatus();
        if (status == CImageStatus::COMPRESSED) {
            return QIcon(":/icons/compression_statuses/compressed.svg");
        } else if (status == CImageStatus::ERROR) {
            return QIcon(":/icons/compression_statuses/error.svg");
        } else if (status == CImageStatus::COMPRESSING) {
            return QIcon(":/icons/compression_statuses/compressing.svg");
        } else {
            return QIcon(":/icons/compression_statuses/uncompressed.svg");
        }
    }

    if (role == Qt::DisplayRole && index.column() == CImageColumns::SIZE) {
        return item->getCImage()->getRichFormattedSize();
    }

    if (role == Qt::DisplayRole && index.column() == CImageColumns::RESOLUTION) {
        return item->getCImage()->getRichResolution();
    }

    if (role == Qt::DisplayRole && index.column() == CImageColumns::RATIO) {
        return item->getCImage()->getRichFormattedSavedRatio();
    }

    return item->data(index.column());
}

Qt::ItemFlags CImageTreeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return QAbstractItemModel::flags(index);
}

QVariant CImageTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return rootItem->data(section);
    }

    return QVariant();
}

//TODO We can maybe optimize these 2 functions to work together
size_t CImageTreeModel::compressedItemsSize() const
{
    QVectorIterator<CImageTreeItem*> itemsIterator(rootItem->children());
    size_t totalSize = 0;
    while (itemsIterator.hasNext()) {
        auto item = itemsIterator.next();
        size_t size = item->getCImage()->getCompressedSize();
        totalSize += size;
    }
    return totalSize;
}

size_t CImageTreeModel::originalItemsSize() const
{
    QVectorIterator<CImageTreeItem*> itemsIterator(rootItem->children());
    size_t totalSize = 0;
    while (itemsIterator.hasNext()) {
        auto item = itemsIterator.next();
        size_t size = item->getCImage()->getOriginalSize();
        totalSize += size;
    }
    return totalSize;
}
