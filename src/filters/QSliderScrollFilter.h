
#ifndef QSLIDERSCROLLFILTER_H
#define QSLIDERSCROLLFILTER_H

#include <QObject>

class QSliderScrollFilter  : public QObject {
    Q_OBJECT

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};



#endif //QSLIDERSCROLLFILTER_H
