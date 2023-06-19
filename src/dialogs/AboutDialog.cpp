#include "AboutDialog.h"
#include "../MainWindow.h"
#include "ui_AboutDialog.h"
#include <QSettings>

#ifdef Q_OS_MAC
#include "./updater/osx/CocoaInitializer.h"
#include "./updater/osx/SparkleAutoUpdater.h"
#endif

#ifdef Q_OS_WIN
#include "./updater/win/winsparkle.h"
#endif

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    QSettings settings;

    ui->version_Label->setText("v" + QCoreApplication::applicationVersion());
    ui->uuid_Label->setText("<small>UUID: " + settings.value("uuid", "").toString() + "</small>");

#ifdef IS_PORTABLE
    ui->checkForUpdates_Button->hide();
#endif

    this->setAttribute(Qt::WA_DeleteOnClose, true);
    this->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    this->setWindowModality(Qt::ApplicationModal);
    this->setAttribute(Qt::WA_DeleteOnClose, false);

    connect(ui->checkForUpdates_Button, &QPushButton::clicked, this, &AboutDialog::onCheckForUpdatesClicked);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::onCheckForUpdatesClicked()
{
#ifdef Q_OS_MAC
    ui->checkForUpdates_Button->setEnabled(false);
    CocoaInitializer initializer;
    auto updater = new SparkleAutoUpdater("https://saerasoft.com/repository/com.saerasoft.caesium/osx/appcast.xml");
    updater->checkForUpdates();
    ui->checkForUpdates_Button->setEnabled(true);
#endif

#ifdef Q_OS_WIN
    win_sparkle_check_update_with_ui();
#endif
}

void AboutDialog::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}
