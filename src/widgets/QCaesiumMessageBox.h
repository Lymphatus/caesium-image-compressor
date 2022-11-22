#ifndef QCAESIUMMESSAGEBOX_H
#define QCAESIUMMESSAGEBOX_H

#include <MainWindow.h>
#include <QMessageBox>

class QCaesiumMessageBox : public QMessageBox {
    Q_OBJECT

public:
    explicit QCaesiumMessageBox(QWidget* parent = nullptr);
};

#endif // QCAESIUMMESSAGEBOX_H
