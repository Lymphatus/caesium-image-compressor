#include "PreferencesDialog.h"
#include "ui_PreferencesDialog.h"

#include "MainWindow.h"
#include "UsageStatsDialog.h"
#include "utils/LanguageManager.h"
#include "utils/Utils.h"
#include <QJsonDocument>
#include <QProcess>
#include <QSettings>


PreferencesDialog::PreferencesDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    this->loadLanguages();
    this->loadThemes();
    this->loadPreferences();

    ui->multithreadingMaxThreads_SpinBox->setMaximum(QThread::idealThreadCount());
    ui->showUsageData_Label->setText(R"(<html><head/><body><p><a href="#"><small style="text-decoration: underline; color:#007af4;">)" + tr("Show usage data") + "</small></a></p></body></html>");

    ui->changesAfterRestartTheme_Label->setVisible(false);
    ui->changesAfterRestartTheme_LabelIcon->setVisible(false);
    ui->restart_Button->setVisible(false);

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
    connect(ui->multithreadingMaxThreads_SpinBox, &QSpinBox::valueChanged, this, &PreferencesDialog::onMultithreadingMaxThreadsChanged);
    connect(ui->showUsageData_Label, &QLabel::linkActivated, this, &PreferencesDialog::onShowUsageDataLinkActivated);
    // connect(ui->skipBySize_CheckBox, &QCheckBox::toggled, this, &PreferencesDialog::onSkipBySizeToggled);
    // connect(ui->skipBySizeCondition_ComboBox, &QComboBox::currentIndexChanged, this, &PreferencesDialog::onSkipBySizeConditionChanged);
    // connect(ui->skipBySizeSize_SpinBox, &QSpinBox::valueChanged, this, &PreferencesDialog::onSkipBySizeValueChanged);
    // connect(ui->skipBySizeUnit_ComboBox, &QComboBox::currentIndexChanged, this, &PreferencesDialog::onSkipBySizeUnitChanged);
    connect(ui->skipCompressionDialogs_CheckBox, &QCheckBox::toggled, this, &PreferencesDialog::onSkipCompressionDialogsToggled);
    connect(ui->postCompressionAction_ComboBox, &QComboBox::currentIndexChanged, this, &PreferencesDialog::onPostCompressionActionChanged);
    connect(ui->restart_Button, &QPushButton::pressed, this, &PreferencesDialog::onRestartButtonPressed);
}

void PreferencesDialog::loadLanguages()
{
    for (const CsLocale& locale : LanguageManager::getSortedTranslations()) {
        ui->language_ComboBox->addItem(locale.label, locale.locale);
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
    ui->checkUpdatesAtStartup_CheckBox->setChecked(settings.value("preferences/general/check_updates_at_startup", true).toBool());
    ui->importSubfolders_CheckBox->setChecked(settings.value("preferences/general/import_subfolders", true).toBool());
    ui->sendUsageReport_CheckBox->setChecked(settings.value("preferences/general/send_usage_reports", true).toBool());
    ui->multithreading_CheckBox->setChecked(settings.value("preferences/general/multithreading", true).toBool());
    ui->multithreadingMaxThreads_SpinBox->setValue(settings.value("preferences/general/multithreading_max_threads", QThread::idealThreadCount()).toInt());
    ui->skipCompressionDialogs_CheckBox->setChecked(settings.value("preferences/general/skip_compression_dialogs", false).toBool());
    ui->theme_ComboBox->setCurrentIndex(settings.value("preferences/general/theme", 0).toInt());
    ui->argsBehaviour_ComboBox->setCurrentIndex(settings.value("preferences/general/args_behaviour", 0).toInt());
    // ui->skipBySize_CheckBox->setChecked(settings.value("preferences/general/skip_by_size/enabled", false).toBool());
    // ui->skipBySizeCondition_ComboBox->setCurrentIndex(settings.value("preferences/general/skip_by_size/condition", 0).toInt());
    // ui->skipBySizeSize_SpinBox->setValue(settings.value("preferences/general/skip_by_size/value", 0).toInt());
    // ui->skipBySizeUnit_ComboBox->setCurrentIndex(settings.value("preferences/general/skip_by_size/unit", 0).toInt());
    ui->postCompressionAction_ComboBox->setCurrentIndex(settings.value("preferences/general/post_compression_action", 0).toInt());
    ui->language_ComboBox->setCurrentIndex(PreferencesDialog::getLocaleIndex());
}

void PreferencesDialog::onPromptExitToggled(bool checked)
{
    QSettings().setValue("preferences/general/prompt_before_exit", checked);
}

void PreferencesDialog::onLanguageChanged(int index)
{
    QString languageId = ui->language_ComboBox->itemData(index).toString();
    QSettings().setValue("preferences/language/locale", languageId);

    auto* p = qobject_cast<MainWindow*>(parent());
    QTranslator* translator = p->getTranslator();
    LanguageManager::loadLocale(translator);
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
    ui->restart_Button->setVisible(true);
    QSettings().setValue("preferences/general/theme", themeIndex);
}

void PreferencesDialog::onArgsBehaviourChanged(int index)
{
    QSettings().setValue("preferences/general/args_behaviour", index);
}

void PreferencesDialog::onShowUsageDataLinkActivated([[maybe_unused]] const QString& link)
{
    auto* usageStatsDialog = new UsageStatsDialog();
    usageStatsDialog->setModal(true);

    usageStatsDialog->show();
}

// void PreferencesDialog::onSkipBySizeToggled(bool checked)
// {
//     QSettings().setValue("preferences/general/skip_by_size/enabled", checked);
// }

// void PreferencesDialog::onSkipBySizeConditionChanged(int index)
// {
//     QSettings().setValue("preferences/general/skip_by_size/condition", index);
// }
//
// void PreferencesDialog::onSkipBySizeValueChanged(int value)
// {
//     QSettings().setValue("preferences/general/skip_by_size/value", value);
// }
//
// void PreferencesDialog::onSkipBySizeUnitChanged(int index)
// {
//     QSettings().setValue("preferences/general/skip_by_size/unit", index);
// }

// Versions below 2.3.0 use an index on the unsorted list, we need to convert it to the new sorting
int PreferencesDialog::getLocaleIndex()
{
    auto localeConf = QSettings().value("preferences/language/locale", "default");
    int localeIndex = 0;
    if (localeConf.type() == QVariant::Int || localeConf.type() == QVariant::LongLong) {
        localeIndex = localeConf.toInt();
        QString locale = LanguageManager::getTranslations().at(localeIndex).locale;
        localeIndex = LanguageManager::findSortedIndex(locale);
    } else if (localeConf.type() == QVariant::String) {
        localeIndex = LanguageManager::findSortedIndex(localeConf.toString());
    }

    return localeIndex;
}

void PreferencesDialog::onSkipCompressionDialogsToggled(bool checked)
{
    QSettings().setValue("preferences/general/skip_compression_dialogs", checked);
}

void PreferencesDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void PreferencesDialog::onMultithreadingMaxThreadsChanged(int value)
{
    QSettings().setValue("preferences/general/multithreading_max_threads", value);
}

void PreferencesDialog::onPostCompressionActionChanged(int value)
{
    QSettings().setValue("preferences/general/post_compression_action", value);
}

void PreferencesDialog::onRestartButtonPressed()
{
    qApp->quit();
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
}
