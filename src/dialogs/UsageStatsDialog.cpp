#include "UsageStatsDialog.h"
#include "ui_UsageStatsDialog.h"
#include "utils/Utils.h"
#include <QJsonDocument>
#include <QSettings>

UsageStatsDialog::UsageStatsDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::UsageStatsDialog)
{
    ui->setupUi(this);

    QFont monospaceFont("Monospace");
    monospaceFont.setStyleHint(QFont::TypeWriter);
    ui->usageData_TextBrowser->setFont(monospaceFont);

    this->showUsageData();
}

UsageStatsDialog::~UsageStatsDialog()
{
    delete ui;
}

void UsageStatsDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void UsageStatsDialog::showUsageData()
{
    QSettings settings;
    QJsonObject compressionData {
        { "uuid", settings.value("uuid").toString() },
        { "totalFiles", "..." },
        { "uncompressedSize", "..." },
        { "compressedSize", "..." },
        { "elapsedTime", "..." },
        { "compressionOptions", getCompressionOptionsAsJSON() }
    };

    QString usageData = tr("System data") + "\n" + QJsonDocument(getSystemData()).toJson(QJsonDocument::Indented) + "\n" + tr("Compression data") + "\n" + QJsonDocument(compressionData).toJson(QJsonDocument::Indented);
    ui->usageData_TextBrowser->setText(usageData);
}
