#include "AboutDialog.h"
#include "../MainWindow.h"
#include "ui_AboutDialog.h"
#include <QProcess>

#ifdef Q_OS_MAC
#include "./updater/osx/CocoaInitializer.h"
#include "./updater/osx/SparkleAutoUpdater.h"
#endif

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    ui->version_Label->setText(QCoreApplication::applicationVersion());

    this->setAttribute(Qt::WA_DeleteOnClose, true);
    this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    this->setWindowModality(Qt::ApplicationModal);
    this->setAttribute(Qt::WA_DeleteOnClose, false);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::on_checkForUpdates_Button_clicked()
{
    #ifdef Q_OS_MAC
    CocoaInitializer initializer;
    auto updater = new SparkleAutoUpdater("https://saerasoft.com/repository/com.saerasoft.caesium/osx/appcast.xml");
    updater->checkForUpdates();
    #endif
}

