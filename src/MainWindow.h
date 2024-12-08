#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QButtonGroup>
#include <QFutureWatcher>
#include <QItemSelection>
#include <QMainWindow>
#include <QShowEvent>
#include <QSystemTrayIcon>
#include <climits>
#include <dialogs/AboutDialog.h>

#include "models/CImageSortFilterProxyModel.h"
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

    [[nodiscard]] QTranslator* getTranslator() const;

protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void on_actionAbout_Caesium_Image_Compressor_triggered() const;
    void on_addFiles_Button_clicked();
    void on_actionAdd_files_triggered();
    void on_compress_Button_clicked();
    void on_actionAdd_folder_triggered();
    void on_removeFiles_Button_clicked();
    void on_outputFolderBrowse_Button_clicked();
    static void on_outputSuffix_LineEdit_textChanged(const QString& arg1);
    void on_actionRemove_triggered();
    void on_actionClear_triggered();
    void on_width_SpinBox_valueChanged(int arg1) const;
    void on_height_SpinBox_valueChanged(int arg1) const;
    static void on_edge_SpinBox_valueChanged(int arg1);
    void on_keepAspectRatio_CheckBox_toggled(bool checked) const;
    void on_doNotEnlarge_CheckBox_toggled(bool checked) const;
    void on_actionSelect_All_triggered() const;
    static void on_sameOutputFolderAsInput_CheckBox_toggled(bool checked);
    static void on_keepStructure_CheckBox_toggled(bool checked);
    void imageList_selectionChanged();
    void compressionFinished();
    void dropFinished(const QStringList& filePaths);
    void on_fitTo_ComboBox_currentIndexChanged(int index) const;
    void on_lossless_CheckBox_toggled(bool checked) const;
    static void on_keepMetadata_CheckBox_toggled(bool checked);
    void onJPEGQualityValueChanged(int value) const;
    void onPNGQualityValueChanged(int value) const;
    void onWebPQualityValueChanged(int value) const;
    void cModelItemsChanged() const;
    void showListContextMenu(const QPoint& pos) const;
    void on_actionShow_previews_toggled(bool arg1) const;
    void on_actionPreferences_triggered();
    void keepDatesButtonGroupClicked() const;
    void on_keepDates_CheckBox_clicked() const;
    static void on_keepDates_CheckBox_stateChanged(int arg1);
    void on_actionShow_original_in_file_manager_triggered() const;
    void on_actionShow_compressed_in_file_manager_triggered() const;
    void listContextMenuAboutToShow() const;
    void showPreview(int index) const;
    void previewFinished() const;
    void previewCanceled() const;
    void compressionCanceled() const;
    static void listSortChanged(int logicalIndex, Qt::SortOrder order);
    void on_actionCompress_triggered();
    static void on_actionDonate_triggered();
    void startCompression(bool onlyfailed = false);
    void on_actionToolbarIcons_only_triggered() const;
    void on_actionToolbarIcons_and_Text_triggered() const;
    void on_actionToolbarHide_triggered() const;
    void updateCompressionProgressLabel(int value) const;
    static void on_actionAuto_preview_toggled(bool toggled);
    void on_actionPreview_triggered() const;
    static void on_skipIfBigger_CheckBox_toggled(bool checked);
    void outputFormatIndexChanged(int index) const;
    void moveOriginalFileToggled(bool checked) const;
    static void moveOriginalFileDestinationChanged(int index);
    void importFromArgs(const QStringList&);
    void onMaxOutputSizeChanged(int value) const;
    void onMaxOutputSizeUnitChanged(int value) const;
    void onCompressionModeChanged(int value) const;
    void onAdvancedImportTriggered();
    void onPNGOptimizationLevelChanged(int value) const;
    void onTIFFCompressionMethodChanged(int index) const;
    static void onTIFFDeflateLevelChanged(int value);
    void onJPEGChromaSubsamplingChanged() const;
    static void onJPEGOptionsVisibilityChanged(bool visible);
    static void onPNGOptionsVisibilityChanged(bool visible);
    static void onWebPOptionsVisibilityChanged(bool visible);
    static void onTIFFOptionsVisibilityChanged(bool visible);
    static void onJPEGProgressiveToggled(bool checked);
    void recompressFailed();
    void installCompressionOptionsEventFilter() const;

private:
    Ui::MainWindow* ui;

    CImageTreeModel* cImageModel;
    QFutureWatcher<void>* compressionWatcher;
    QFutureWatcher<ImagePreview>* previewWatcher;
    QMap<QString, int> folderMap;
    AboutDialog* aboutDialog = nullptr;
    QString lastOpenedDirectory;
    QString importedFilesRootFolder;
    QMenu* listContextMenu {};
    QMenu* trayIconContextMenu {};
    QButtonGroup* keepDatesButtonGroup;
    CompressionSummary compressionSummary;
    NetworkOperations* networkOperations;
    QElapsedTimer compressionTimer;
    CImageSortFilterProxyModel* proxyModel;
    long long selectedCount = 0;
    QList<QModelIndex> selectedIndexes;
    bool isItemRemovalRunning = false;
    QSystemTrayIcon* trayIcon;
    QTranslator* translator;

    void initStatusBar() const;
    static void initUpdater();
    void initListContextMenu();
    void initTrayIconContextMenu();
    void initListWidget() const;
    void initTrayIcon();

    void toggleUIEnabled(bool enabled) const;
    void importFiles(const QStringList& fileList, QString baseFolder);
    void removeFiles(bool all = false);
    void triggerImportFiles();
    void triggerImportFolder();
    void writeSettings() const;
    void readSettings();
    void previewImage(const QModelIndex& imageIndex, bool forceRuntimePreview = false) const;
    void updateFolderMap(QString baseFolder, int count);
    void toggleLosslessWarningVisible() const;
    CompressionOptions getCompressionOptions(const QString& rootFolder) const;
    static void clearCache();
    void changeEvent(QEvent* event) override;
    void setupChromaSubsamplingComboBox() const;
    void setupCompressButton();
};

#endif // MAINWINDOW_H
