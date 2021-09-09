#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QThread>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = 0);
    ~AboutDialog();

private slots:
    void on_checkForUpdates_Button_clicked();
public slots:
    void checkForUpdatesStarted();
    void updateIsAvailable(const QString &filePath);
    void checkForUpdatesFinished();

private:
    Ui::AboutDialog *ui;
    QThread updaterThread;
    bool hasPendingUpdate = false;
    QString updateFilePath;
};

#endif // ABOUTDIALOG_H
