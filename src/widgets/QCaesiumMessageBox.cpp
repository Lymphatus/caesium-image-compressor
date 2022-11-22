#include "QCaesiumMessageBox.h"

QCaesiumMessageBox::QCaesiumMessageBox(QWidget* parent)
    : QMessageBox(parent)
{
    QPixmap icon = QPixmap(":/icons/logo.png").scaledToHeight((int)ceil(72 * this->devicePixelRatio()), Qt::SmoothTransformation);
    icon.setDevicePixelRatio(this->devicePixelRatio());
    setIconPixmap(icon);
}