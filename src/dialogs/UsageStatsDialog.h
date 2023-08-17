#ifndef USAGESTATSDIALOG_H
#define USAGESTATSDIALOG_H

#include <QDialog>


namespace Ui {
class UsageStatsDialog;
}

class UsageStatsDialog : public QDialog {
    Q_OBJECT

public:
    explicit UsageStatsDialog(QWidget* parent = nullptr);
    ~UsageStatsDialog() override;

private:
    Ui::UsageStatsDialog* ui;

    void showUsageData();
    void changeEvent(QEvent* event) override;
};

#endif // USAGESTATSDIALOG_H
