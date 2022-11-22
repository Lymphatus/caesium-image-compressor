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
    this->loadThemes();
    this->loadPreferences();

    ui->menu_ListWidget->setCurrentRow(0);
    ui->showUsageData_Label->setText(R"(<html><head/><body><p><a href="#"><small style="text-decoration: underline; color:#007af4;">)" + tr("Show usage data") + "</small></a></p></body></html>");

    ui->changesAfterRestartTheme_Label->setVisible(false);
    ui->changesAfterRestartTheme_LabelIcon->setVisible(false);

    this->setupConnections();
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::setupConnections()
{
    connect(ui->language_ComboBox, &QComboBox::currentIndexChanged, this, &PreferencesDialog::onLanguageChanged);
    connect(ui->theme_ComboBox, &QComboBox::currentIndexChanged, this, &PreferencesDialog::onThemeChanged);
    connect(ui->argsBehaviour_ComboBox, &QComboBox::currentIndexChanged, this, &PreferencesDialog::onArgsBehaviourChanged);
    connect(ui->promptExit_CheckBox, &QCheckBox::toggled, this, &PreferencesDialog::onPromptExitToggled);
    connect(ui->checkUpdatesAtStartup_CheckBox, &QCheckBox::toggled, this, &PreferencesDialog::onCheckUpdatesAtStartupToggled);
    connect(ui->importSubfolders_CheckBox, &QCheckBox::toggled, this, &PreferencesDialog::onImportSubfoldersToggled);
    connect(ui->sendUsageReport_CheckBox, &QCheckBox::toggled, this, &PreferencesDialog::onSendUsageReportToggled);
    connect(ui->multithreading_CheckBox, &QCheckBox::toggled, this, &PreferencesDialog::onMultithreadingToggled);
    connect(ui->showUsageData_Label, &QLabel::linkActivated, this, &PreferencesDialog::onShowUsageDataLinkActivated);
    connect(ui->skipBySize_CheckBox, &QCheckBox::toggled, this, &PreferencesDialog::onSkipBySizeToggled);
    connect(ui->skipBySizeCondition_ComboBox, &QComboBox::currentIndexChanged, this, &PreferencesDialog::onSkipBySizeConditionChanged);
    connect(ui->skipBySizeSize_SpinBox, &QSpinBox::valueChanged, this, &PreferencesDialog::onSkipBySizeValueChanged);
    connect(ui->skipBySizeUnit_ComboBox, &QComboBox::currentIndexChanged, this, &PreferencesDialog::onSkipBySizeUnitChanged);
}

void PreferencesDialog::loadLanguages()
{
    for (const CsLocale& locale : LANGUAGES) {
        ui->language_ComboBox->addItem(locale.label);
    }
}

void PreferencesDialog::loadThemes()
{
    for (const CsTheme& theme : THEMES) {
        ui->theme_ComboBox->addItem(theme.label);
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
    ui->argsBehaviour_ComboBox->setCurrentIndex(settings.value("preferences/general/args_behaviour", 0).toInt());
    ui->skipBySize_CheckBox->setChecked(settings.value("preferences/general/skip_by_size/enabled", false).toBool());
    ui->skipBySizeCondition_ComboBox->setCurrentIndex(settings.value("preferences/general/skip_by_size/condition", 0).toInt());
    ui->skipBySizeSize_SpinBox->setValue(settings.value("preferences/general/skip_by_size/value", 0).toInt());
    ui->skipBySizeUnit_ComboBox->setCurrentIndex(settings.value("preferences/general/skip_by_size/unit", 0).toInt());
}

void PreferencesDialog::onPromptExitToggled(bool checked)
{
    QSettings().setValue("preferences/general/prompt_before_exit", checked);
}

void PreferencesDialog::onLanguageChanged(int index)
{
    int localeIndex = index;
    if (index < 0 || index > LANGUAGES_COUNT - 1) {
        localeIndex = 0;
    }
    ui->changesAfterRestartTheme_Label->setVisible(true);
    ui->changesAfterRestartTheme_LabelIcon->setVisible(true);
    QSettings().setValue("preferences/language/locale", localeIndex);
}

void PreferencesDialog::onCheckUpdatesAtStartupToggled(bool checked)
{
    QSettings().setValue("preferences/general/check_updates_at_startup", checked);
}

void PreferencesDialog::onImportSubfoldersToggled(bool checked)
{
    QSettings().setValue("preferences/general/import_subfolders", checked);
}

void PreferencesDialog::onSendUsageReportToggled(bool checked)
{
    QSettings().setValue("preferences/general/send_usage_reports", checked);
}

void PreferencesDialog::onMultithreadingToggled(bool checked)
{
    QSettings().setValue("preferences/general/multithreading", checked);
}

void PreferencesDialog::onThemeChanged(int index)
{
    int themeIndex = index;
    if (index < 0 || index > THEMES_COUNT - 1) {
        themeIndex = 0;
    }
    ui->changesAfterRestartTheme_Label->setVisible(true);
    ui->changesAfterRestartTheme_LabelIcon->setVisible(true);
    QSettings().setValue("preferences/general/theme", themeIndex);
}

void PreferencesDialog::onArgsBehaviourChanged(int index)
{
    QSettings().setValue("preferences/general/args_behaviour", index);
}

void PreferencesDialog::onShowUsageDataLinkActivated(const QString& link)
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
    QMessageBox messageBox;
    messageBox.setStyleSheet("QLabel{min-width: 500px;}");
    messageBox.setText(tr("Usage data"));
    messageBox.setInformativeText(tr("This data is collected to provide the best long term support for the application. No data is sent to third parties."));
    messageBox.setDetailedText(tr("System data") + "\n" + QJsonDocument(getSystemData()).toJson(QJsonDocument::Indented) + "\n" + tr("Compression data") + "\n" + QJsonDocument(compressionData).toJson(QJsonDocument::Indented));
    messageBox.setStandardButtons(QMessageBox::Ok);
    messageBox.setDefaultButton(QMessageBox::Ok);
    messageBox.exec();
}

void PreferencesDialog::onSkipBySizeToggled(bool checked)
{
    QSettings().setValue("preferences/general/skip_by_size/enabled", checked);
}

void PreferencesDialog::onSkipBySizeConditionChanged(int index)
{
    QSettings().setValue("preferences/general/skip_by_size/condition", index);
}

void PreferencesDialog::onSkipBySizeValueChanged(int value)
{
    QSettings().setValue("preferences/general/skip_by_size/value", value);
}

void PreferencesDialog::onSkipBySizeUnitChanged(int index)
{
    QSettings().setValue("preferences/general/skip_by_size/unit", index);
}
