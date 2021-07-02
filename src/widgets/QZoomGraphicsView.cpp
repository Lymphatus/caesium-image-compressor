#include "QZoomGraphicsView.h"

#include <QDebug>

QZoomGraphicsView::QZoomGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
}

void QZoomGraphicsView::wheelEvent(QWheelEvent* event)
{
    this->setScaleFactor(event);
    emit scaleFactorChanged(event);
}

float QZoomGraphicsView::getScaleFactor() const
{
    return scaleFactor;
}

void QZoomGraphicsView::setScaleFactor(QWheelEvent* event)
{
    const ViewportAnchor anchor = transformationAnchor();
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    int angle = event->angleDelta().y();

    qreal factor = 1;
    if ((float) angle > WHEEL_TOLERANCE) {
        factor = ZOOM_IN_RATIO;
    } else if ((float) angle < -WHEEL_TOLERANCE) {
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
