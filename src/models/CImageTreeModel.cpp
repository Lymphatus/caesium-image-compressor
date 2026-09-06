#include "CImageTreeModel.h"

#include <QApplication>
#include <QIcon>
#include <QLabel>
#include <QPropertyAnimation>
#include <QStyle>

CImageTreeModel::CImageTreeModel()
{
    rootItem = new CImageTreeItem({ tr("Name"), tr("Size"), tr("Resolution"), tr("Saved"), tr("Info") });
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

void CImageTreeModel::appendItems(QList<CImage*> imageList, QString folder)
{
    updatePalette();
    if (this->baseFolder != folder) {
        this->baseFolder = folder;
        for (CImageTreeItem* item : rootItem->children()) {
            updateRelativeFolder(item);
        }
        if (rowCount() > 0) {
            emit dataChanged(index(0, 0), index(rowCount() - 1, 0));
        }
    }
    this->setupModelData(imageList, rootItem);
}

void CImageTreeModel::setupModelData(const QList<CImage*> imageList, CImageTreeItem* parent)
{
    if (imageList.isEmpty()) {
        return;
    }
    QListIterator<CImage*> iterator(imageList);
    this->beginInsertRows(QModelIndex(), this->rowCount(), this->rowCount() + imageList.count() - 1);
    while (iterator.hasNext()) {
        CImage* nextImage = iterator.next();
        auto* cImageTreeItem = new CImageTreeItem(nextImage, parent);
        updateRelativeFolder(cImageTreeItem);
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

    if (role == Qt::DisplayRole && index.column() == CImageColumns::NAME_COLUMN) {
        updatePalette();
        return item->displayName();
    }

    if (role == Qt::DecorationRole && index.column() == CImageColumns::NAME_COLUMN) {
        if (statusPixmaps.isEmpty()) {
            const QStringList names = { "uncompressed", "compressing", "compressed", "error", "warning" };
            for (const QString& name : names) {
                statusPixmaps.append(QIcon(":/icons/compression_statuses/" + name + ".svg").pixmap(16, 16));
            }
        }
        return statusPixmaps.at(static_cast<int>(item->displayedStatus()));
    }

    if (role == Qt::DisplayRole && index.column() == CImageColumns::SIZE_COLUMN) {
        return item->cachedRichSize();
    }

    if (role == Qt::DisplayRole && index.column() == CImageColumns::RESOLUTION_COLUMN) {
        return item->cachedRichResolution();
    }

    if (role == Qt::DisplayRole && index.column() == CImageColumns::RATIO_COLUMN) {
        return item->cachedRatioText();
    }

    if (role == Qt::DisplayRole && index.column() == CImageColumns::INFO_COLUMN) {
        return item->cachedInfoText();
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

double CImageTreeModel::compressedItemsSize() const
{
    QVectorIterator<CImageTreeItem*> itemsIterator(rootItem->children());
    double totalSize = 0;
    while (itemsIterator.hasNext()) {
        auto item = itemsIterator.next();
        auto size = (double)item->getCImage()->getCompressedSize();
        totalSize += size;
    }
    return totalSize;
}

double CImageTreeModel::originalItemsSize() const
{
    QVectorIterator<CImageTreeItem*> itemsIterator(rootItem->children());
    double totalSize = 0;
    while (itemsIterator.hasNext()) {
        auto item = itemsIterator.next();
        auto size = (double)item->getCImage()->getOriginalSize();
        totalSize += size;
    }
    return totalSize;
}

void CImageTreeModel::updateRelativeFolder(CImageTreeItem* item)
{
    QString fullPath = item->getCImage()->getFullPath();
    QString computedBaseFolder = fullPath.remove(baseFolder + "/");
    item->setRelativeFolder(computedBaseFolder.remove(item->getCImage()->getFileName()));
    updateDisplayName(item);
}

void CImageTreeModel::updateDisplayName(CImageTreeItem* item) const
{
    item->setDisplayName("<span style=\"color:" + rgbaString + ";\">" + item->relativeFolder() + "</span>" + item->getCImage()->getFileName());
}

void CImageTreeModel::updatePalette() const
{
    const QPalette palette = QApplication::palette();
    if (paletteKey == palette.cacheKey()) {
        return;
    }
    paletteKey = palette.cacheKey();
    const QColor defaultColor = palette.text().color();
    rgbaString = "rgba(" + QString::number(defaultColor.red()) + "," + QString::number(defaultColor.green()) + "," + QString::number(defaultColor.blue()) + ",.6);";
    for (CImageTreeItem* item : rootItem->children()) {
        updateDisplayName(item);
    }
}
