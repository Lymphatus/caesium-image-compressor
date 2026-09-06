#ifndef CIMAGETREEMODEL_H
#define CIMAGETREEMODEL_H

#include "CImage.h"
#include "CImageTreeItem.h"

#include <QAbstractItemModel>
#include <QDir>
#include <QHash>

class CImageTreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit CImageTreeModel();
    ~CImageTreeModel();

    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeItems(QList<int> rows);
    bool isCompressing() const;

    void appendItems(QList<CImage*> imageList, QString folder = "");

    CImageTreeItem* getRootItem() const;
    bool contains(CImage* cImage);

    double originalItemsSize() const;
    double compressedItemsSize() const;

private:
    void setupModelData(const QList<CImage*> imageList, CImageTreeItem* parent);
    void updateRelativeFolder(CImageTreeItem* item);
    void updateDisplayName(CImageTreeItem* item) const;
    void updatePalette() const;

    CImageTreeItem* rootItem;
    QString baseFolder;
    QHash<QString, int> fullPathRefCount;
    QFuture<void> compressionFuture;
    mutable qint64 paletteKey = 0;
    mutable QString rgbaString;
    mutable QVector<QPixmap> statusPixmaps;

signals:
    void itemsChanged();

public slots:
    void emitDataChanged(int row);
};

#endif // CIMAGETREEMODEL_H
