#ifndef CIMAGESORTFILTERPROXYMODEL_H
#define CIMAGESORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

class CImageSortFilterProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
private:
    static int naturalCompare(const QString& left, const QString& right);

public:
    explicit CImageSortFilterProxyModel(QObject* parent = nullptr);

    bool lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const override;
};

#endif // CIMAGESORTFILTERPROXYMODEL_H
