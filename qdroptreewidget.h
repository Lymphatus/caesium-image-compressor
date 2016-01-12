#ifndef QDROPTREEWIDGET_H
#define QDROPTREEWIDGET_H

#include <caesium.h>

#include <QTreeWidget>

class QMimeData;
class Caesium;

class QDropTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    QDropTreeWidget(QWidget *parent = 0);

signals:
    void dropFinished(QStringList);
    void itemsChanged();

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

};

#endif // QDROPTREEWIDGET_H
