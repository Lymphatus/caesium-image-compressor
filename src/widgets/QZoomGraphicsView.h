#include <climits>
#ifndef QZOOMGRAPHICSVIEW_H
#define QZOOMGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>
#include <QMouseEvent>

class QZoomGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit QZoomGraphicsView(QWidget *parent = 0);
    void wheelEvent(QWheelEvent* event) override;
    void resetScaleFactor();

private:
    const float WHEEL_TOLERANCE = 1; //Experimental for touchpads
    const float ZOOM_IN_RATIO = 1.05;
    const float ZOOM_OUT_RATIO = 0.95;
    const float MAX_ZOOM_IN = 10;
    const float MAX_ZOOM_OUT = 0.1;
    bool zooming = false;

public:
    bool isZooming() const;

private:
    float scaleFactor = 1;

public slots:
    void setScaleFactor(QWheelEvent* event);
    void setHorizontalScrollBarValue(int);
    void setVerticalScrollBarValue(int);

signals:
    void scaleFactorChanged(QWheelEvent* event);
    void mouseMoved(QMouseEvent* event);
};

#endif // QZOOMGRAPHICSVIEW_H
