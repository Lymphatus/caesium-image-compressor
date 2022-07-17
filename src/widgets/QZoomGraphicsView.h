#include <climits>
#ifndef QZOOMGRAPHICSVIEW_H
#define QZOOMGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>
#include <QMovie>
#include <QGraphicsProxyWidget>


class QZoomGraphicsView : public QGraphicsView {
    Q_OBJECT

public:
    explicit QZoomGraphicsView(QWidget* parent = nullptr);
    void wheelEvent(QWheelEvent* event) override;
    void resetScaleFactor();
    void setLoading(bool l);
    void setZoomEnabled(bool l);
    void showPixmap(QPixmap pixmap);
    void removePixmap();

private:
    const float WHEEL_TOLERANCE = 1; // Experimental for touchpads
    const float ZOOM_IN_RATIO = 1.05;
    const float ZOOM_OUT_RATIO = 0.95;
    const float MAX_ZOOM_IN = 10;
    const float MAX_ZOOM_OUT = 0.1;
    float scaleFactor = 1;
    bool zooming = false;
    bool loading = false;
    bool zoomEnabled = true;

    QLabel* loaderLabel;
    QMovie* loaderMovie;
    QGraphicsProxyWidget *loaderProxyWidget;
    QGraphicsScene* graphicsScene;
    QGraphicsPixmapItem* pixmapItem;

public slots:
    void setScaleFactor(QWheelEvent* event);
    void setHorizontalScrollBarValue(int);
    void setVerticalScrollBarValue(int);

signals:
    void scaleFactorChanged(QWheelEvent* event);
};

#endif // QZOOMGRAPHICSVIEW_H
