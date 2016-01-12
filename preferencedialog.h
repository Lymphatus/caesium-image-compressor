#ifndef PREFERENCEDIALOG_H
#define PREFERENCEDIALOG_H

#include <QMainWindow>

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
#define KEY_PREF_GENERAL_PROMPT QString("promptExit")

//Compression group keys
#define KEY_PREF_COMPRESSION_EXIF QString("exif")
#define KEY_PREF_COMPRESSION_EXIF_COPYRIGHT QString("exifCopyright")
#define KEY_PREF_COMPRESSION_EXIF_DATE QString("exifDate")
#define KEY_PREF_COMPRESSION_EXIF_COMMENT QString("exifComment")
#define KEY_PREF_COMPRESSION_PROGRESSIVE QString("progressive")

//Privacy group keys
#define KEY_PREF_PRIVACY_USAGE QString("sendInfo")

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
    void on_actionCompression_triggered();
    void on_actionGeneral_triggered();
    void on_actionPrivacy_triggered();
    void closeEvent(QCloseEvent *event);
    void writePreferences();
    void readPreferences();

    void on_seeInfoButton_clicked();
    void on_outputFileMethodComboBox_currentIndexChanged(int index);
    void on_browseButton_clicked();
    void on_keepCopyrightCheckBox_toggled(bool checked);
    void on_keepDateCheckBox_toggled(bool checked);
    void on_keepCommentsCheckBox_toggled(bool checked);

private:
    Ui::PreferenceDialog *ui;
    enum Qt::CheckState getExifsCheckBoxGroupState();

};

#endif // PREFERENCEDIALOG_H
