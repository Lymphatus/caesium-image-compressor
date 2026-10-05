#ifndef CIMAGETREEMODEL_H
#define CIMAGETREEMODEL_H

#include "CImage.h"
#include "CImageTreeItem.h"

#include <QAbstractItemModel>
#include <QDir>
#include <QFuture>
#include <QHash>
#include <QMutex>
#include <QThreadPool>
#include <QTimer>
#include <atomic>

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
    QFuture<void> compress(QThreadPool* pool, const CompressionOptions& options, bool onlyFailed = false);
    void cancelCompression();

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
    void scheduleFlush();

    CImageTreeItem* rootItem;
    QString baseFolder;
    QHash<QString, int> fullPathRefCount;
    QFuture<void> compressionFuture;
    std::atomic_bool compressionCanceled { false };
    QList<int> batchRows;
    QVector<CImageTreeItem*> batchItems;
    QMutex pendingMutex;
    QList<int> pendingStarted;
    QList<int> pendingFinished;
    QTimer updateTimer;
    mutable qint64 paletteKey = 0;
    mutable QString rgbaString;
    mutable QVector<QPixmap> statusPixmaps;

signals:
    void itemsChanged();
    void itemCompressionStarted(int row);
    void itemCompressionFinished(int row);

public slots:
    void emitDataChanged(int row);
    void flushPendingUpdates();
};

#endif // CIMAGETREEMODEL_H
