#include "CImageSortFilterProxyModel.h"
#include "models/CImageTreeItem.h"
#include "utils/Utils.h"
#include <QCollator>

CImageSortFilterProxyModel::CImageSortFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel { parent }
{
}

bool CImageSortFilterProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    auto* leftItem = static_cast<CImageTreeItem*>(left.internalPointer());
    auto* rightItem = static_cast<CImageTreeItem*>(right.internalPointer());

    CImage* leftCImage = leftItem->getCImage();
    CImage* rightCImage = rightItem->getCImage();

    if (left.column() == CImageColumns::NAME_COLUMN && right.column() == CImageColumns::NAME_COLUMN) {
        return naturalCompare(leftItem->displayName(), rightItem->displayName()) < 0;
    } else if (left.column() == CImageColumns::SIZE_COLUMN && right.column() == CImageColumns::SIZE_COLUMN) {
        return leftCImage->getOriginalSize() < rightCImage->getOriginalSize();
    } else if (left.column() == CImageColumns::RESOLUTION_COLUMN && right.column() == CImageColumns::RESOLUTION_COLUMN) {
        return leftCImage->getTotalPixels() < rightCImage->getTotalPixels();
    } else if (left.column() == CImageColumns::RATIO_COLUMN && right.column() == CImageColumns::RATIO_COLUMN) {
        if (leftItem->cachedCompressedSize() == 0 && rightItem->cachedCompressedSize() == 0) {
            return naturalCompare(leftItem->cachedRatioText(), rightItem->cachedRatioText()) < 0;
        }

        return leftItem->cachedRatio() < rightItem->cachedRatio();
    }
    return naturalCompare(leftItem->cachedInfoText(), rightItem->cachedInfoText()) < 0;
}

int CImageSortFilterProxyModel::naturalCompare(const QString& left, const QString& right)
{
    static thread_local QCollator collator = [] {
        QCollator result;
        result.setCaseSensitivity(Qt::CaseSensitive);
        result.setNumericMode(true);
        return result;
    }();
    return collator.compare(left, right);
}
