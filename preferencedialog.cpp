#include "preferencedialog.h"
#include "ui_preferencedialog.h"
#include "usageinfo.h"
#include "caesium.h"
#include "utils.h"

#include <QCloseEvent>
#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>

PreferenceDialog::PreferenceDialog(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PreferenceDialog) {
    ui->setupUi(this);
    readPreferences();

    //If we want a custom folder, show the browse button
    ui->browseButton->setVisible(ui->outputFileMethodComboBox->currentIndex() == 2);
}

PreferenceDialog::~PreferenceDialog() {
    delete ui;
}

void PreferenceDialog::on_actionCompression_triggered() {
    ui->stackedWidget->setCurrentIndex(1);
}

void PreferenceDialog::on_actionGeneral_triggered() {
    ui->stackedWidget->setCurrentIndex(0);
}

void PreferenceDialog::on_actionPrivacy_triggered() {
    ui->stackedWidget->setCurrentIndex(2);
}

void PreferenceDialog::closeEvent(QCloseEvent *event) {
    writePreferences();
    event->accept();
}

void PreferenceDialog::writePreferences() {
    QSettings settings;

    //General
    settings.beginGroup(KEY_PREF_GROUP_GENERAL);
    settings.setValue(KEY_PREF_GENERAL_OVERWRITE, ui->overwriteOriginalCheckBox->isChecked());
    settings.setValue(KEY_PREF_GENERAL_SUBFOLDER, ui->subfoldersCheckBox->isChecked());
    settings.setValue(KEY_PREF_GENERAL_OUTPUT_METHOD, ui->outputFileMethodComboBox->currentIndex());
    settings.setValue(KEY_PREF_GENERAL_OUTPUT_STRING, ui->outputFileMethodLineEdit->text());
    settings.setValue(KEY_PREF_GENERAL_PROMPT, ui->promptExitCheckBox->isChecked());
    settings.setValue(KEY_PREF_GENERAL_LOCALE, ui->languageComboBox->currentIndex());
    settings.endGroup();

    //Compression
    settings.beginGroup(KEY_PREF_GROUP_COMPRESSION);
    settings.setValue(KEY_PREF_COMPRESSION_EXIF, ui->exifCheckBox->checkState());
    settings.setValue(KEY_PREF_COMPRESSION_EXIF_COPYRIGHT, ui->keepCopyrightCheckBox->isChecked());
    settings.setValue(KEY_PREF_COMPRESSION_EXIF_DATE, ui->keepDateCheckBox->isChecked());
    settings.setValue(KEY_PREF_COMPRESSION_EXIF_COMMENT, ui->keepCommentsCheckBox->isChecked());
    settings.setValue(KEY_PREF_COMPRESSION_PROGRESSIVE, ui->progressiveCheckBox->isChecked());
    settings.endGroup();

    //Privacy
    settings.beginGroup(KEY_PREF_GROUP_PRIVACY);
    settings.setValue(KEY_PREF_PRIVACY_USAGE, ui->sendInfoCheckBox->isChecked());
    settings.endGroup();
}

void PreferenceDialog::readPreferences() {
    QSettings settings;

    //General
    settings.beginGroup(KEY_PREF_GROUP_GENERAL);
    ui->overwriteOriginalCheckBox->setChecked(settings.value(KEY_PREF_GENERAL_OVERWRITE).value<bool>());
    ui->subfoldersCheckBox->setChecked(settings.value(KEY_PREF_GENERAL_SUBFOLDER).value<bool>());
    ui->outputFileMethodComboBox->setCurrentIndex(settings.value(KEY_PREF_GENERAL_OUTPUT_METHOD).value<int>());
    ui->outputFileMethodLineEdit->setText(settings.value(KEY_PREF_GENERAL_OUTPUT_STRING).value<QString>());
    ui->promptExitCheckBox->setChecked(settings.value(KEY_PREF_GENERAL_PROMPT).value<bool>());
    ui->languageComboBox->setCurrentIndex(settings.value(KEY_PREF_GENERAL_LOCALE).value<int>());
    settings.endGroup();

    //Compression
    settings.beginGroup(KEY_PREF_GROUP_COMPRESSION);
    qDebug() << settings.value(KEY_PREF_COMPRESSION_EXIF).value<Qt::CheckState>();
    ui->exifCheckBox->setCheckState(settings.value(KEY_PREF_COMPRESSION_EXIF).value<Qt::CheckState>());
    ui->keepCopyrightCheckBox->setChecked(settings.value(KEY_PREF_COMPRESSION_EXIF_COPYRIGHT).value<bool>());
    ui->keepDateCheckBox->setChecked(settings.value(KEY_PREF_COMPRESSION_EXIF_DATE).value<bool>());
    ui->keepCommentsCheckBox->setChecked(settings.value(KEY_PREF_COMPRESSION_EXIF_COMMENT).value<bool>());
    ui->progressiveCheckBox->setChecked(settings.value(KEY_PREF_COMPRESSION_PROGRESSIVE).value<bool>());
    settings.endGroup();

    //Privacy
    settings.beginGroup(KEY_PREF_GROUP_PRIVACY);
    ui->seeInfoButton->setChecked(settings.value(KEY_PREF_PRIVACY_USAGE).value<bool>());
    settings.endGroup();
}

void PreferenceDialog::on_seeInfoButton_clicked() {
    QMessageBox msgBox;
    msgBox.setText(tr("This data will help improve this application and won't be shared with anyone."));
    msgBox.setDetailedText(uinfo->printJSON());
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    //Fixed size hack
    QSpacerItem* horizontalSpacer = new QSpacerItem(400, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout* layout = (QGridLayout*)msgBox.layout();
    layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());
    msgBox.exec();
}

void PreferenceDialog::on_outputFileMethodComboBox_currentIndexChanged(int index) {
    ui->outputFileMethodLineEdit->setReadOnly(index == 2);
    ui->browseButton->setVisible(index == 2);
    switch(index) {
    case 0:
        ui->outputFileMethodLineEdit->setText(tr("_compressed"));
        break;
    case 1:
        ui->outputFileMethodLineEdit->setText(tr("compressed"));
        break;
    case 2:
        ui->outputFileMethodLineEdit->setText(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) +
                                              "/Caesium");
        break;
    default:
        break;
    }
}

void PreferenceDialog::on_browseButton_clicked() {
    ui->outputFileMethodLineEdit->setText(
                QFileDialog::getExistingDirectory(this,
                                                  tr("Select an output folder..."),
                                                  QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                                  QFileDialog::ShowDirsOnly));
}

void PreferenceDialog::on_keepCopyrightCheckBox_toggled(bool checked) {
    ui->exifCheckBox->setCheckState(getExifsCheckBoxGroupState());
}

void PreferenceDialog::on_keepDateCheckBox_toggled(bool checked) {
    ui->exifCheckBox->setCheckState(getExifsCheckBoxGroupState());
}

void PreferenceDialog::on_keepCommentsCheckBox_toggled(bool checked) {
    ui->exifCheckBox->setCheckState(getExifsCheckBoxGroupState());
}

enum Qt::CheckState PreferenceDialog::getExifsCheckBoxGroupState() {
    if (ui->keepDateCheckBox->isChecked() &&
            ui->keepCommentsCheckBox->isChecked() &&
            ui->keepCopyrightCheckBox->isChecked()) {
        //All selected
        return Qt::Checked;
    } else if (ui->keepDateCheckBox->isChecked() ||
               ui->keepCommentsCheckBox->isChecked() ||
               ui->keepCopyrightCheckBox->isChecked()) {
        //At least one is selected
        return Qt::PartiallyChecked;
    } else {
        //None is selected
        return Qt::Unchecked;
    }
}
