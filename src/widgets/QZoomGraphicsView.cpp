#include "QZoomGraphicsView.h"

#include <QScrollBar>

QZoomGraphicsView::QZoomGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
}

void QZoomGraphicsView::wheelEvent(QWheelEvent* event)
{
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

    float expectedScaleFactor = this->scaleFactor * factor;
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

bool QZoomGraphicsView::isZooming() const
{
    return zooming;
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
void QZoomGraphicsView::drawForeground(QPainter* painter, const QRectF& rect)
{
    painter->resetTransform();

    if (this->loading) {
        painter->setOpacity(.3);
        QString loadingText = tr("Loading...");
        auto fontRect = painter->fontMetrics().boundingRect(loadingText);
        painter->drawText(this->rect().right() - fontRect.width() - 5, this->rect().bottom() - painter->fontMetrics().leading() - 5, loadingText);
    }
    QGraphicsView::drawForeground(painter, rect);
}

void QZoomGraphicsView::setLoading(bool loading)
{
    QZoomGraphicsView::loading = loading;
}
