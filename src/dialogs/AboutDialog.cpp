#include "AboutDialog.h"
#include "ui_AboutDialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    ui->version_Label->setText(QCoreApplication::applicationVersion());
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
