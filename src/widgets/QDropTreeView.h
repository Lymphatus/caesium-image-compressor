#include <climits>
#ifndef QDROPTREEWIDGET_H
#define QDROPTREEWIDGET_H

#include <QTreeView>

class QMimeData;

class QDropTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit QDropTreeView(QWidget *parent = 0);

signals:
    void dropFinished(QStringList);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

} __unused;

#endif // QDROPTREEWIDGET_H
