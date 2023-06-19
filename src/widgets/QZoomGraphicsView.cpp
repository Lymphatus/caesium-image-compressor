#include "QZoomGraphicsView.h"

#include <QLabel>
#include <QMovie>
#include <QPainterPath>
#include <QScrollBar>

QZoomGraphicsView::QZoomGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
    this->graphicsScene = new QGraphicsScene();
    this->setScene(this->graphicsScene);

    this->loaderLabel = new QLabel();
    this->loaderLabel->setAutoFillBackground(false);
    this->loaderLabel->setAttribute(Qt::WA_NoSystemBackground);
    this->loaderMovie = new QMovie(":/icons/ui/loader.webp");
    this->loaderMovie->setScaledSize(QSize(40, 40));
    this->loaderLabel->setMovie(this->loaderMovie);
    this->loaderMovie->start();
    this->loaderProxyWidget = this->graphicsScene->addWidget(this->loaderLabel);
    this->loaderProxyWidget->hide();

    this->pixmapItem = new QGraphicsPixmapItem();
}

void QZoomGraphicsView::wheelEvent(QWheelEvent* event)
{
    if (!this->zoomEnabled) {
        return;
    }
    this->zooming = true;
    this->setScaleFactor(event);
    emit scaleFactorChanged(event);
    this->zooming = false;
    emit this->horizontalScrollBar()->valueChanged(this->horizontalScrollBar()->value());
    emit this->verticalScrollBar()->valueChanged(this->verticalScrollBar()->value());
}

void QZoomGraphicsView::setScaleFactor(QWheelEvent* event)
{
    const ViewportAnchor anchor = transformationAnchor();
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    int angle = event->angleDelta().y();

    qreal factor = 1;
    if ((float)angle > WHEEL_TOLERANCE) {
        factor = ZOOM_IN_RATIO;
    } else if ((float)angle < -WHEEL_TOLERANCE) {
        factor = ZOOM_OUT_RATIO;
    }

    double expectedScaleFactor = this->scaleFactor * factor;
    if (expectedScaleFactor > MAX_ZOOM_IN || expectedScaleFactor < MAX_ZOOM_OUT) {
        return;
    }

    setTransformationAnchor(anchor);
    scale(factor, factor);

    this->scaleFactor *= factor;
}

void QZoomGraphicsView::resetScaleFactor()
{
    this->scaleFactor = 1;
}

void QZoomGraphicsView::setHorizontalScrollBarValue(int value)
{
    if (!this->zooming) {
        this->horizontalScrollBar()->setValue(value);
    }
}

void QZoomGraphicsView::setVerticalScrollBarValue(int value)
{
    if (!this->zooming) {
        this->verticalScrollBar()->setValue(value);
    }
}

void QZoomGraphicsView::setLoading(bool l)
{
    this->loading = l;

    if (l) {
        this->graphicsScene->setSceneRect(0, 0, 40, 40);
        this->fitInView(this->rect(), Qt::KeepAspectRatio);
        this->loaderProxyWidget->show();
    } else {
        this->loaderProxyWidget->hide();
    }
}

void QZoomGraphicsView::setZoomEnabled(bool l)
{
    this->zoomEnabled = l;
}

void QZoomGraphicsView::showPixmap(const QPixmap& pixmap)
{
    this->pixmapItem = this->graphicsScene->addPixmap(pixmap);
    this->graphicsScene->setSceneRect(this->graphicsScene->itemsBoundingRect());
}

void QZoomGraphicsView::removePixmap()
{
    qsizetype indexOf = this->graphicsScene->items().indexOf(this->pixmapItem);
    if (indexOf != -1) {
        auto* item = this->graphicsScene->items().at(indexOf);
        delete item;
    }
}
