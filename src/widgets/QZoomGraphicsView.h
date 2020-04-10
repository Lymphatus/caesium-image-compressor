#ifndef QZOOMGRAPHICSVIEW_H
#define QZOOMGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>

class QZoomGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit QZoomGraphicsView(QWidget *parent = 0);
    void wheelEvent(QWheelEvent* event);
};

#endif // QZOOMGRAPHICSVIEW_H
