/*
 *
 * This file is part of Caesium Image Compressor.
 *
 * Caesium Image Compressor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Caesium Image Compressor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Caesium Image Compressor.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include <QDebug>
#include <QCloseEvent>
#include <QSettings>
#include <QStandardPaths>
#include <QFileDialog>
#include <QDir>

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PreferencesDialog)
{
    this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    this->settings = new QSettings();
    ui->setupUi(this);

    initizializeUI();
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::closeEvent(QCloseEvent *event) {
    this->writePreferences();
    event->accept();
}

void PreferencesDialog::writePreferences()
{
    qInfo() << "Writing settings to" << this->settings->fileName();
    //General
    this->settings->beginGroup(KEY_PREF_GROUP_GENERAL);
    this->settings->setValue(KEY_PREF_GENERAL_OVERWRITE, ui->overwrite_CheckBox->isChecked());
    this->settings->setValue(KEY_PREF_GENERAL_SUBFOLDER, ui->scanSubfolders_CheckBox->isChecked());
    this->settings->setValue(KEY_PREF_GENERAL_OUTPUT_FOLDER, ui->outputFolder_LineEdit->text());
    this->settings->setValue(KEY_PREF_GENERAL_OUTPUT_SUFFIX, ui->suffix_LineEdit->text());
    this->settings->setValue(KEY_PREF_GENERAL_SAME_AS_INPUT, ui->sameInputFolder_CheckBox->isChecked());
    this->settings->setValue(KEY_PREF_GENERAL_SKIP_GREATER, ui->skip_CheckBox->isChecked());
    this->settings->setValue(KEY_PREF_GENERAL_PROMPT, ui->promptBeforeExit_CheckBox->isChecked());
    this->settings->setValue(KEY_PREF_GENERAL_LOCALE, ui->locale_ComboBox->currentIndex());
    this->settings->endGroup();
}

void PreferencesDialog::readPreferences()
{
    qInfo() << "Reading settings from" << this->settings->fileName();
    //General
    this->settings->beginGroup(KEY_PREF_GROUP_GENERAL);
    ui->overwrite_CheckBox->setChecked(this->settings->value(KEY_PREF_GENERAL_OVERWRITE).value<bool>());
    ui->scanSubfolders_CheckBox->setChecked(this->settings->value(KEY_PREF_GENERAL_SUBFOLDER).value<bool>());
    ui->outputFolder_LineEdit->setText(this->settings->value(KEY_PREF_GENERAL_OUTPUT_FOLDER).value<QString>());
    ui->suffix_LineEdit->setText(this->settings->value(KEY_PREF_GENERAL_OUTPUT_SUFFIX).value<QString>());
    ui->sameInputFolder_CheckBox->setChecked(this->settings->value(KEY_PREF_GENERAL_SAME_AS_INPUT).value<bool>());
    ui->skip_CheckBox->setChecked(this->settings->value(KEY_PREF_GENERAL_SKIP_GREATER).value<bool>());
    ui->promptBeforeExit_CheckBox->setChecked(this->settings->value(KEY_PREF_GENERAL_PROMPT).value<bool>());
    ui->locale_ComboBox->setCurrentIndex(this->settings->value(KEY_PREF_GENERAL_LOCALE).value<int>());
    this->settings->endGroup();

    //Defaults/fallbacks
    if (ui->outputFolder_LineEdit->text().isEmpty()) {
        ui->outputFolder_LineEdit->setText(
                    QDir::toNativeSeparators(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0) + "/Caesium"));
    }
}

void PreferencesDialog::initizializeUI()
{
    this->readPreferences();
}

void PreferencesDialog::on_outputFolderBrowse_Button_clicked()
{
    QString selectedFolder = QFileDialog::getExistingDirectory(this,
                                                               tr("Select an output folder..."),
                                                               QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
                                                               QFileDialog::ShowDirsOnly);
    if (!selectedFolder.isEmpty()) {
        ui->outputFolder_LineEdit->setText(selectedFolder);
    }
}
