#include "QZoomGraphicsView.h"

#include <QDebug>

QZoomGraphicsView::QZoomGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
}

void QZoomGraphicsView::wheelEvent(QWheelEvent* event)
{
    const ViewportAnchor anchor = transformationAnchor();
    setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    int angle = event->angleDelta().y();

    qreal factor = 1;
    if (angle > WHEEL_TOLERANCE) {
        factor = ZOOM_IN_RATIO;
    } else if (angle < -WHEEL_TOLERANCE) {
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

float QZoomGraphicsView::getScaleFactor() const
{
    return scaleFactor;
}
