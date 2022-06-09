#ifndef QCAESIUMMESSAGEBOX_H
#define QCAESIUMMESSAGEBOX_H

#include <MainWindow.h>
#include <QMessageBox>

class QCaesiumMessageBox : public QMessageBox {
    Q_OBJECT

public:
    explicit QCaesiumMessageBox(QWidget* parent = 0);
    explicit QCaesiumMessageBox(Icon icon, const QString& title, const QString& text,
        StandardButtons buttons = NoButton, QWidget* parent = nullptr,
        Qt::WindowFlags flags = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
};

#endif // QCAESIUMMESSAGEBOX_H
