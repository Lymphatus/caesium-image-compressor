#include "AboutDialog.h"
#include "../MainWindow.h"
#include "ui_AboutDialog.h"
#include "utils/Updater.h"
#include <QProcess>

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    ui->version_Label->setText(QCoreApplication::applicationVersion());
    auto pbSizePolicy = ui->checkForUpdates_ProgressBar->sizePolicy();
    pbSizePolicy.setRetainSizeWhenHidden(true);
    ui->checkForUpdates_ProgressBar->setSizePolicy(pbSizePolicy);
    ui->checkForUpdates_ProgressBar->setVisible(false);
    auto slSizePolicy = ui->updateStatus_Label->sizePolicy();
    slSizePolicy.setRetainSizeWhenHidden(true);
    ui->updateStatus_Label->setSizePolicy(slSizePolicy);
    ui->updateStatus_Label->setVisible(false);

    this->setAttribute(Qt::WA_DeleteOnClose, true);
    this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    this->setWindowModality(Qt::ApplicationModal);
    this->setAttribute(Qt::WA_DeleteOnClose, false);

    connect(&updaterThread, &QThread::started, this, &AboutDialog::checkForUpdatesStarted);

}

AboutDialog::~AboutDialog()
{
    if (!this->updaterThread.isFinished()) {
        //Try to exit cleanly
        this->updaterThread.quit();
        this->updaterThread.wait();
    }
    delete ui;
}

void AboutDialog::on_checkForUpdates_Button_clicked()
{
    if (this->hasPendingUpdate) {
        QString currentProcessPath = QCoreApplication::applicationFilePath();
        Updater::replaceCurrentFiles(this->updateFilePath);
        QProcess::startDetached(currentProcessPath, QCoreApplication::arguments());
        QCoreApplication::quit();
    } else {
        auto* updater = new Updater();
        updater->moveToThread(&updaterThread);
        connect(&updaterThread, &QThread::finished, updater, &QObject::deleteLater);
        connect(updater, &Updater::resultReady, this, &AboutDialog::updateIsAvailable);
        connect(updater, &Updater::finished, this, &AboutDialog::checkForUpdatesFinished);
        connect(updater, &Updater::resultReady, dynamic_cast<MainWindow*>(this->parent()), &MainWindow::updateAvailable);
        updaterThread.start();
        updater->checkForUpdates();
    }
}

void AboutDialog::checkForUpdatesStarted()
{
    ui->updateStatus_Label->setVisible(false);
    ui->checkForUpdates_ProgressBar->setVisible(true);
    ui->checkForUpdates_Button->setDisabled(true);
    ui->checkForUpdates_ProgressBar->setRange(0, 0);
}

void AboutDialog::updateIsAvailable(const QString &filePath)
{
    this->hasPendingUpdate = true;
    ui->updateStatus_Label->setVisible(false);
    ui->checkForUpdates_ProgressBar->setVisible(false);
    ui->checkForUpdates_Button->setDisabled(false);
    ui->checkForUpdates_Button->setText(tr("Restart to update"));
    this->updateFilePath = filePath;
}
void AboutDialog::checkForUpdatesFinished()
{
    if (this->hasPendingUpdate) {
        return;
    }

    updaterThread.quit();
    updaterThread.wait();
    this->hasPendingUpdate = false;
    ui->updateStatus_Label->setVisible(true);
    ui->checkForUpdates_ProgressBar->setVisible(false);
    ui->checkForUpdates_Button->setDisabled(false);
}
