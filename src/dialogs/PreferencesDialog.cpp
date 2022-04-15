#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"

#include "utils/Utils.h"
#include <QJsonDocument>
#include <QMessageBox>
#include <QSettings>

PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    this->loadLanguages();
    this->loadPreferences();

    ui->menu_ListWidget->setCurrentRow(0);
    ui->showUsageData_Label->setText("<html><head/><body><p><a href=\"#\"><small style=\"text-decoration: underline; color:#007af4;\">" + tr("Show usage data") + "</small></a></p></body></html>");

    connect(ui->language_ComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(languageComboBoxIndexChanged(int)));
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::loadLanguages()
{
    for (const CsLocale &locale : LANGUAGES) {
        ui->language_ComboBox->addItem(locale.label);
    }
}

void PreferencesDialog::loadPreferences()
{
    QSettings settings;
    ui->promptExit_CheckBox->setChecked(settings.value("preferences/general/prompt_before_exit", false).toBool());
    ui->checkUpdatesAtStartup_CheckBox->setChecked(settings.value("preferences/general/check_updates_at_startup", false).toBool());
    ui->importSubfolders_CheckBox->setChecked(settings.value("preferences/general/import_subfolders", true).toBool());
    ui->sendUsageReport_CheckBox->setChecked(settings.value("preferences/general/send_usage_reports", true).toBool());
    ui->multithreading_CheckBox->setChecked(settings.value("preferences/general/multithreading", true).toBool());
    ui->language_ComboBox->setCurrentIndex(settings.value("preferences/language/locale", 0).toInt());
    ui->theme_ComboBox->setCurrentIndex(settings.value("preferences/general/theme", 0).toInt());
}

void PreferencesDialog::writeSetting(const QString& key, const QVariant& value)
{
    QSettings settings;
    settings.setValue(key, value);
}

void PreferencesDialog::on_promptExit_CheckBox_toggled(bool checked)
{
    this->writeSetting("preferences/general/prompt_before_exit", checked);
}

void PreferencesDialog::languageComboBoxIndexChanged(int index)
{
    int localeIndex = index;
    if (index < 0 || index > LANGUAGES_COUNT - 1) {
        localeIndex = 0;
    }
    this->writeSetting("preferences/language/locale", localeIndex);
}

void PreferencesDialog::on_checkUpdatesAtStartup_CheckBox_toggled(bool checked)
{
    this->writeSetting("preferences/general/check_updates_at_startup", checked);
}

void PreferencesDialog::on_importSubfolders_CheckBox_toggled(bool checked)
{
    this->writeSetting("preferences/general/import_subfolders", checked);
}

void PreferencesDialog::on_sendUsageReport_CheckBox_toggled(bool checked)
{
    this->writeSetting("preferences/general/send_usage_reports", checked);
}

void PreferencesDialog::on_multithreading_CheckBox_toggled(bool checked)
{
    this->writeSetting("preferences/general/multithreading", checked);
}

void PreferencesDialog::on_theme_ComboBox_currentIndexChanged(int index)
{
    this->writeSetting("preferences/general/theme", index);
}

void PreferencesDialog::on_showUsageData_Label_linkActivated(const QString& link)
{
    QSettings settings;
    QJsonObject compressionData {
        { "uuid", settings.value("uuid").toString() },
        { "totalFiles", "..." },
        { "uncompressedSize", "..." },
        { "compressedSize", "..." },
        { "elapsedTime", "..." },
    };
    QMessageBox messageBox;
    messageBox.setStyleSheet("QLabel{min-width: 500px;}");
    messageBox.setText(tr("Usage data"));
    messageBox.setInformativeText(tr("This data is collected to provide the best long term support for the application. No data is sent to third parties."));
    messageBox.setDetailedText(tr("System data") + "\n" + QJsonDocument(getSystemData()).toJson(QJsonDocument::Indented) + "\n" + tr("Compression data") + "\n" + QJsonDocument(compressionData).toJson(QJsonDocument::Indented));
    messageBox.setStandardButtons(QMessageBox::Ok);
    messageBox.setDefaultButton(QMessageBox::Ok);
    messageBox.exec();
}
