#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget* parent = nullptr);
    ~PreferencesDialog() override;

private slots:
    void onLanguageChanged(int index) const;
    void onThemeChanged(int index) const;
    void onThemeVariantChanged(int index) const;
    static int getLocaleIndex();
    static void onArgsBehaviourChanged(int index);
    static void onPromptExitToggled(bool checked);
    static void onCheckUpdatesAtStartupToggled(bool checked);
    static void onImportSubfoldersToggled(bool checked);
    static void onSendUsageReportToggled(bool checked);
    static void onMultithreadingToggled(bool checked);
    static void onShowUsageDataLinkActivated([[maybe_unused]] const QString& link);
    static void onSkipCompressionDialogsToggled(bool checked);
    static void onMultithreadingMaxThreadsChanged(int value);
    static void onPostCompressionActionChanged(int value);
    static void onRestartButtonPressed();
    static void onThreadsPriorityChanged(int value);

private:
    Ui::PreferencesDialog* ui;

    void setupConnections();
    void loadPreferences() const;
    void loadLanguages() const;
    void loadThemes() const;
    void changeEvent(QEvent* event) override;
};

#endif // PREFERENCESDIALOG_H
