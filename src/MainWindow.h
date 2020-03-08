#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QFutureWatcher>
#include <QGraphicsScene>
#include <QMainWindow>

#include <src/models/CImageTreeModel.h>

#include <src/widgets/QTreeViewItemDelegate.h>

#include "src/models/CImage.h"

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
    void resizeEvent(QResizeEvent* event) override;

private slots:

    void on_actionAbout_Caesium_Image_Compressor_triggered();
    void on_addFiles_Button_clicked();
    void on_actionAdd_files_triggered();
    void on_compress_Button_clicked();
    void on_actionAdd_folder_triggered();
    void on_removeFiles_Button_clicked();
    void on_outputFolderBrowse_Button_clicked();
    void on_outputSuffix_LineEdit_textChanged(const QString& arg1);
    void on_lossless_Checkbox_stateChanged(int arg1);
    void on_keepMetadata_Checkbox_stateChanged(int arg1);
    void on_compression_Slider_valueChanged(int value);

    void imageList_selectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void compressionFinished();

private:
    Ui::MainWindow* ui;

    CImageTreeModel* cImageModel;
    QTreeViewItemDelegate* listViewDelegate;
    QFutureWatcher<void> compressionWatcher;
    QGraphicsScene* previewScene;

    void importFiles(const QStringList& fileList);
    void triggerImportFiles();
    void triggerImportFolder();
    void writeSettings();
    void writeSetting(const QString& key, const QVariant& value);
    void readSettings();
    void previewImage(const QModelIndex &imageIndex);
    QVariant readSetting(const QString& key);
};

#endif // MAINWINDOW_H
