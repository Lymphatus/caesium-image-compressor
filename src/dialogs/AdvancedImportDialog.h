#ifndef ADVANCEDIMPORTDIALOG_H
#define ADVANCEDIMPORTDIALOG_H

#include <QDialog>
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

private:
    Ui::AdvancedImportDialog* ui;

    void changeEvent(QEvent* event) override;
};

#endif // ADVANCEDIMPORTDIALOG_H
