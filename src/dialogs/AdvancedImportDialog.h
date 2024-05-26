#ifndef ADVANCEDIMPORTDIALOG_H
#define ADVANCEDIMPORTDIALOG_H

#include <QDialog>
#include <QFileInfo>
#include <QThread>

namespace Ui {
class AdvancedImportDialog;
}

class AdvancedImportDialog : public QDialog {
    Q_OBJECT

public:
    explicit AdvancedImportDialog(QWidget* parent = nullptr);
    ~AdvancedImportDialog() override;

signals:
    void importTriggered(QStringList paths);

private slots:
    void accept() override;
    void onAddFilesActionTriggered();
    void onAddFolderActionTriggered();
    void onImportFromListActionTriggered();
    void onRemoveButtonClicked() const;
    static void onImportSubfoldersToggled(bool checked);
    static void onSkipBySizeToggled(bool checked);
    static void onSkipBySizeConditionChanged(int index);
    static void onSkipBySizeValueChanged(int value);
    static void onSkipBySizeUnitChanged(int index);
    static void onFilenamePatternTextChanged(const QString& text);
    void dropFinished(QDropEvent* event) const;


private:
    Ui::AdvancedImportDialog* ui;

    void changeEvent(QEvent* event) override;
    void setupAddButtonMenu() const;
    void setupConnections();
    void loadPreferences();
    void setLastOpenedDirectory(const QString& directory);
    static QStringList openList(const QString& filePath);

    QMenu* addButtonContextMenu;
    QAction* addFilesAction;
    QAction* addFolderAction;
    QAction* importFromListAction;
    QString lastOpenedDirectory;
};

#endif // ADVANCEDIMPORTDIALOG_H
