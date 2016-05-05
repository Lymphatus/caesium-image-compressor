/**
 *
 * This file is part of Caesium.
 *
 * Caesium - Caesium is an image compression software aimed at helping photographers,
 * bloggers, webmasters, businesses or casual users at storing, sending and sharing digital pictures.
 *
 * Copyright (C) 2016 - Matteo Paonessa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>
 *
 */

#include "preferencedialog.h"
#include "ui_preferencedialog.h"
#include "caesium.h"
#include "utils.h"

#include <QCloseEvent>
#include <QSettings>
#include <QMessageBox>
#include <QFileDialog>
#include <QLibraryInfo>
#include <QDirIterator>
#include <QStyledItemDelegate>
#include <QTranslator>
#include <QStandardPaths>

PreferenceDialog::PreferenceDialog(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PreferenceDialog) {

    this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    ui->setupUi(this);
    QSettings settings;
    loadTranslations();
    readPreferences();

    //If we want a custom folder, show the browse button
    ui->browseButton->setVisible(ui->outputFileMethodComboBox->currentIndex() == 2);

    //Override the item delegate for styling QComboBox on OSX
    QStyledItemDelegate* itemDelegate = new QStyledItemDelegate();
    ui->languageComboBox->setItemDelegate(itemDelegate);
    ui->outputFileMethodComboBox->setItemDelegate(itemDelegate);

    //Initial list state
    ui->menuListWidget->setCurrentRow(ui->stackedWidget->currentIndex());
}

PreferenceDialog::~PreferenceDialog() {
    delete ui;
}

void PreferenceDialog::closeEvent(QCloseEvent *event) {
    writePreferences();
    event->accept();
}

void PreferenceDialog::writePreferences() {
    //General
    settings.beginGroup(KEY_PREF_GROUP_GENERAL);
    settings.setValue(KEY_PREF_GENERAL_OVERWRITE, ui->overwriteOriginalCheckBox->isChecked());
    settings.setValue(KEY_PREF_GENERAL_SUBFOLDER, ui->subfoldersCheckBox->isChecked());
    settings.setValue(KEY_PREF_GENERAL_OUTPUT_METHOD, ui->outputFileMethodComboBox->currentIndex());
    settings.setValue(KEY_PREF_GENERAL_OUTPUT_STRING, ui->outputFileMethodLineEdit->text());
    settings.setValue(KEY_PREF_GENERAL_PROMPT, ui->promptExitCheckBox->isChecked());
    settings.setValue(KEY_PREF_GENERAL_LOCALE, ui->languageComboBox->currentIndex());
    settings.endGroup();
}

void PreferenceDialog::readPreferences() {
    //General
    settings.beginGroup(KEY_PREF_GROUP_GENERAL);
    ui->overwriteOriginalCheckBox->setChecked(settings.value(KEY_PREF_GENERAL_OVERWRITE).value<bool>());
    ui->subfoldersCheckBox->setChecked(settings.value(KEY_PREF_GENERAL_SUBFOLDER).value<bool>());
    ui->outputFileMethodComboBox->setCurrentIndex(settings.value(KEY_PREF_GENERAL_OUTPUT_METHOD).value<int>());
    ui->outputFileMethodLineEdit->setText(settings.value(KEY_PREF_GENERAL_OUTPUT_STRING).value<QString>());
    ui->promptExitCheckBox->setChecked(settings.value(KEY_PREF_GENERAL_PROMPT).value<bool>());
    ui->languageComboBox->setCurrentIndex(settings.value(KEY_PREF_GENERAL_LOCALE).value<int>());
    settings.endGroup();
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

/*void PreferenceDialog::on_keepCopyrightCheckBox_toggled(bool checked) {
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
}*/

void PreferenceDialog::loadTranslations() {
    for (int i = 1; i < locales.length(); i++) {
        ui->languageComboBox->addItem(toCapitalCase(locales.at(i).nativeLanguageName()));
    }
}

void PreferenceDialog::on_languageComboBox_currentIndexChanged(int index) {
    qDebug() << "Writing to settings language" << locales.at(index).name();
    settings.setValue(KEY_PREF_GENERAL_LOCALE_STRING, locales.at(index).name());
}


void PreferenceDialog::on_menuListWidget_currentRowChanged(int currentRow) {
    ui->stackedWidget->setCurrentIndex(currentRow);
}
