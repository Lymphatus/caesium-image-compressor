#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QThread>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog {
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr);
    ~AboutDialog() override;

private slots:
    void onCheckForUpdatesClicked();

private:
    Ui::AboutDialog* ui;

    void changeEvent(QEvent* event) override;
};

#endif // ABOUTDIALOG_H
