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
#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QMainWindow>
#include <QString>
#include <QSettings>

//Preference group keys
#define KEY_PREF_GROUP_GENERAL QString("PreferenceGeneral/")
#define KEY_PREF_GROUP_COMPRESSION QString("PreferenceCompression/")
#define KEY_PREF_GROUP_PRIVACY QString("PreferencePrivacy/")
#define KEY_PREF_GROUP_GEOMETRY QString("WindowGeometry/")

//General group keys
#define KEY_PREF_GENERAL_OVERWRITE QString("overwrite")
#define KEY_PREF_GENERAL_SUBFOLDER QString("scanSubfolders")
#define KEY_PREF_GENERAL_OUTPUT_FOLDER QString("outputFolder")
#define KEY_PREF_GENERAL_OUTPUT_SUFFIX QString("outputSuffix")
#define KEY_PREF_GENERAL_SAME_AS_INPUT QString("sameAsInput")
#define KEY_PREF_GENERAL_SKIP_GREATER QString("skipGreater")
#define KEY_PREF_GENERAL_LOCALE QString("locale")
#define KEY_PREF_GENERAL_LOCALE_STRING QString("localeString")
#define KEY_PREF_GENERAL_PROMPT QString("promptExit")

//Geometry group keys
#define KEY_PREF_GEOMETRY_SIZE QString("size")
#define KEY_PREF_GEOMETRY_POS QString("pos")
#define KEY_PREF_GEOMETRY_SORT_ORDER QString("sortOrder")
#define KEY_PREF_GEOMETRY_SORT_COLUMN QString("sortColumn")

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QMainWindow
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = 0);
    ~PreferencesDialog();

private:
    Ui::PreferencesDialog *ui;
    void writePreferences();
    void readPreferences();
    void initizializeUI();

    QSettings *settings;

private slots:
    void closeEvent(QCloseEvent *event);
    void on_outputFolderBrowse_Button_clicked();
};

#endif // PREFERENCESDIALOG_H
