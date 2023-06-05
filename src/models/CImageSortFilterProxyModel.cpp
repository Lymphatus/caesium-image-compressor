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
    QVariant leftData = sourceModel()->data(left);
    QVariant rightData = sourceModel()->data(right);

    CImage* leftCImage = static_cast<CImageTreeItem*>(left.internalPointer())->getCImage();
    CImage* rightCImage = static_cast<CImageTreeItem*>(right.internalPointer())->getCImage();

    if (left.column() == CImageColumns::NAME_COLUMN && right.column() == CImageColumns::NAME_COLUMN) {
        // TODO Needs a regex for the HTML field?
        return naturalLessThan(leftData.toString(), rightData.toString());
    } else if (left.column() == CImageColumns::SIZE_COLUMN && right.column() == CImageColumns::SIZE_COLUMN) {
        return leftCImage->getOriginalSize() < rightCImage->getOriginalSize();
    } else if (left.column() == CImageColumns::RESOLUTION_COLUMN && right.column() == CImageColumns::RESOLUTION_COLUMN) {
        return leftCImage->getTotalPixels() < rightCImage->getTotalPixels();
    } else if (left.column() == CImageColumns::RATIO_COLUMN && right.column() == CImageColumns::RATIO_COLUMN) {
        if (leftCImage->getCompressedSize() == 0 && rightCImage->getCompressedSize() == 0) {
            return naturalLessThan(leftData.toString(), rightData.toString());
        }

        return leftCImage->getRatio() < rightCImage->getRatio();
    }
    return naturalLessThan(leftData.toString(), rightData.toString());
}

bool CImageSortFilterProxyModel::naturalLessThan(QString left, QString right)
{
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseSensitive);
    collator.setNumericMode(true);
    return collator.compare(left, right) < 0;
}
