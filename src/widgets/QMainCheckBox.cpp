#include "QMainCheckBox.h"

QMainCheckBox::QMainCheckBox(QWidget* parent)
    : QCheckBox(parent)
{
}

void QMainCheckBox::nextCheckState()
{
    if (checkState() == Qt::PartiallyChecked) {
        setCheckState(Qt::Checked);
    } else {
        setChecked(!isChecked());
    }
}
