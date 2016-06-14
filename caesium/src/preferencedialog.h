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

#ifndef PREFERENCEDIALOG_H
#define PREFERENCEDIALOG_H

#include <QMainWindow>
#include <QSettings>

//Preference group keys
#define KEY_PREF_GROUP_GENERAL QString("PreferenceGeneral/")
#define KEY_PREF_GROUP_COMPRESSION QString("PreferenceCompression/")
#define KEY_PREF_GROUP_PRIVACY QString("PreferencePrivacy/")
#define KEY_PREF_GROUP_GEOMETRY QString("WindowGeometry/")

//General group keys
#define KEY_PREF_GENERAL_OVERWRITE QString("overwrite")
#define KEY_PREF_GENERAL_SUBFOLDER QString("subfolder")
#define KEY_PREF_GENERAL_OUTPUT_METHOD QString("outMethod")
#define KEY_PREF_GENERAL_OUTPUT_STRING QString("outString")
#define KEY_PREF_GENERAL_LOCALE QString("locale")
#define KEY_PREF_GENERAL_LOCALE_STRING QString("localeString")
#define KEY_PREF_GENERAL_PROMPT QString("promptExit")

//Geometry group keys
#define KEY_PREF_GEOMETRY_SIZE QString("size")
#define KEY_PREF_GEOMETRY_POS QString("pos")
#define KEY_PREF_GEOMETRY_PANEL_VISIBLE QString("sidePanelVisible")
#define KEY_PREF_GEOMETRY_PANEL_FLOAT QString("sidePanelFloating")
#define KEY_PREF_GEOMETRY_SORT_ORDER QString("sortOrder")
#define KEY_PREF_GEOMETRY_SORT_COLUMN QString("sortColumn")

namespace Ui {
class PreferenceDialog;
}

class PreferenceDialog : public QMainWindow
{
    Q_OBJECT

public:
    explicit PreferenceDialog(QWidget *parent = 0);
    ~PreferenceDialog();

private slots:
    void closeEvent(QCloseEvent *event);
    void writePreferences();
    void readPreferences();
    void loadTranslations();

    void on_outputFileMethodComboBox_currentIndexChanged(int index);
    void on_browseButton_clicked();
    //void on_keepCopyrightCheckBox_toggled(bool checked);
    //void on_keepDateCheckBox_toggled(bool checked);
    //void on_keepCommentsCheckBox_toggled(bool checked);
    void on_languageComboBox_currentIndexChanged(int index);

    void on_menuListWidget_currentRowChanged(int currentRow);

private:
    Ui::PreferenceDialog *ui;
    enum Qt::CheckState getExifsCheckBoxGroupState();
    QSettings settings;

};

#endif // PREFERENCEDIALOG_H
