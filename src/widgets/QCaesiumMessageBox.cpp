#include "QCaesiumMessageBox.h"

QCaesiumMessageBox::QCaesiumMessageBox(QWidget* parent)
    : QMessageBox(parent)
{
    QPixmap icon = QPixmap(":/icons/logo.png").scaledToHeight((int)ceil(72 * this->devicePixelRatio()), Qt::SmoothTransformation);
    icon.setDevicePixelRatio(this->devicePixelRatio());
    setIconPixmap(icon);
}

QCaesiumMessageBox::QCaesiumMessageBox(QMessageBox::Icon icon, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QWidget* parent, Qt::WindowFlags flags)
    : QMessageBox(icon, title, text, buttons, parent, flags)
{
    QPixmap i = QPixmap(":/icons/logo.png").scaledToHeight((int)ceil(72 * this->devicePixelRatio()), Qt::SmoothTransformation);
    i.setDevicePixelRatio(this->devicePixelRatio());
    setIconPixmap(i);
}