#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <climits>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QDialog>
#include <QFutureWatcher>
#include <QGraphicsScene>
#include <QItemSelection>
#include <QMainWindow>
#include <QShowEvent>
#include <QNetworkAccessManager>
#include <dialogs/AboutDialog.h>

#include "models/CImage.h"
#include "models/CImageTreeModel.h"
#include "network/NetworkOperations.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void on_actionAbout_Caesium_Image_Compressor_triggered();
    void on_addFiles_Button_clicked();
    void on_actionAdd_files_triggered();
    void on_compress_Button_clicked();
    void on_actionAdd_folder_triggered();
    void on_removeFiles_Button_clicked();
    void on_outputFolderBrowse_Button_clicked();
    void on_outputSuffix_LineEdit_textChanged(const QString& arg1);
    void on_actionRemove_triggered();
    void on_actionClear_triggered();
    void on_width_SpinBox_valueChanged(int arg1);
    void on_height_SpinBox_valueChanged(int arg1);
    void on_edge_SpinBox_valueChanged(int arg1);
    void on_keepAspectRatio_CheckBox_toggled(bool checked);
    void on_doNotEnlarge_CheckBox_toggled(bool checked);
    void on_actionSelect_All_triggered();
    void on_sameOutputFolderAsInput_CheckBox_toggled(bool checked);
    void on_keepStructure_CheckBox_toggled(bool checked);
    void imageList_selectionChanged();
    void compressionFinished();
    void dropFinished(QStringList filePaths);
    void on_fitTo_ComboBox_currentIndexChanged(int index);
    void on_lossless_CheckBox_toggled(bool checked);
    void on_keepMetadata_CheckBox_toggled(bool checked);
    void on_JPEGQuality_Slider_valueChanged(int value);
    void on_PNGLevel_Slider_valueChanged(int value);
    void on_JPEGQuality_SpinBox_valueChanged(int arg1);
    void on_PNGLevel_SpinBox_valueChanged(int arg1);
    void cModelItemsChanged();
    void showListContextMenu(const QPoint &pos);
    void on_actionShow_previews_toggled(bool arg1);
    void on_actionPreferences_triggered();
    void keepDatesButtonGroupClicked();

    void on_keepDates_CheckBox_clicked();

    void on_keepDates_CheckBox_stateChanged(int arg1);

private:
    Ui::MainWindow* ui;

    CImageTreeModel* cImageModel;
    QFutureWatcher<void>* compressionWatcher;
    QGraphicsScene* previewScene;
    QGraphicsScene* compressedPreviewScene;
    QMap<QString, int> folderMap;
    AboutDialog* aboutDialog = nullptr;
    QString lastOpenedDirectory;
    QString importedFilesRootFolder;
    QMenu* listContextMenu{};
    QButtonGroup* keepDatesButtonGroup;
    CompressionSummary compressionSummary;
    NetworkOperations* networkOperations;
    QElapsedTimer compressionTimer;

    void initStatusBar();
    void initUpdater();
    void initListContextMenu();
    void initListWidget();

    void importFiles(const QStringList& fileList, QString baseFolder);
    void removeFiles(bool all = false);
    void triggerImportFiles();
    void triggerImportFolder();
    void writeSettings();
    void writeSetting(const QString& key, const QVariant& value);
    void readSettings();
    void previewImage(const QModelIndex& imageIndex);
    void updateFolderMap(QString baseFolder, int count);
};

#endif // MAINWINDOW_H
