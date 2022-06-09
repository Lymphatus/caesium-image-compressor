#ifndef QMAINCHECKBOX_H
#define QMAINCHECKBOX_H

#include <QCheckBox>

class QMainCheckBox : public QCheckBox {
    Q_OBJECT

public:
    explicit QMainCheckBox(QWidget* parent = nullptr);

protected:
    void nextCheckState() override;
};

#endif // QMAINCHECKBOX_H
