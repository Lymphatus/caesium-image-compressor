#include "AdvancedImportDialog.h"
#include "ui_AdvancedImportDialog.h"

AdvancedImportDialog::AdvancedImportDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AdvancedImportDialog)
{
    ui->setupUi(this);
}

AdvancedImportDialog::~AdvancedImportDialog()
{
    delete ui;
}

void AdvancedImportDialog::accept()
{
    QDialog::accept();

    auto list = QStringList() << "Test" << "Test2";
    emit importTriggered(list);
}

void AdvancedImportDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}
