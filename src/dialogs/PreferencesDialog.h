#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);
    ~PreferencesDialog();

private slots:
    void on_promptExit_CheckBox_toggled(bool checked);
    void languageComboBoxIndexChanged(int index);
    void themeComboBoxIndexChanged(int index);
    void on_checkUpdatesAtStartup_CheckBox_toggled(bool checked);
    void on_importSubfolders_CheckBox_toggled(bool checked);
    void on_sendUsageReport_CheckBox_toggled(bool checked);
    void on_multithreading_CheckBox_toggled(bool checked);
    void on_showUsageData_Label_linkActivated(const QString &link);

private:
    Ui::PreferencesDialog *ui;

    void loadPreferences();
    void loadLanguages();
    void loadThemes();
    void writeSetting(const QString& key, const QVariant& value);
};

#endif // PREFERENCESDIALOG_H
