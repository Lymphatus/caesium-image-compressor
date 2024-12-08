
#include "QSliderScrollFilter.h"
#include <QKeyEvent>

bool QSliderScrollFilter::eventFilter(QObject* obj, QEvent* event)
{

    if (event->type() == QEvent::Wheel) {
        QCoreApplication::sendEvent(obj->parent(), event);
        return true;
    }
    return QObject::eventFilter(obj, event);
}