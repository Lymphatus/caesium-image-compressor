#include "MainWindow.h"
#include "src/dialogs/AboutDialog.h"
#include "src/utils/Utils.h"
#include "ui_MainWindow.h"

#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFuture>
#include <QList>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QThread>
#include <QThreadPool>
#include <QtConcurrent>

#include <src/models/CImageTreeModel.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->cImageModel = new CImageTreeModel();
    this->previewScene = new QGraphicsScene();
    ui->preview_graphicsView->setScene(this->previewScene);
    //    this->listViewDelegate = new QTreeViewItemDelegate(ui->imageList_TreeView, this);
    ui->imageList_TreeView->setModel(this->cImageModel);
    ui->imageList_TreeView->setIconSize(QSize(10, 10));
    ui->imageList_TreeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    //    ui->imageList_TreeView->setItemDelegate(listViewDelegate);

    connect(ui->imageList_TreeView->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(imageList_selectionChanged(const QModelIndex &, const QModelIndex &)));
    this->readSettings();
}

MainWindow::~MainWindow()
{
    delete cImageModel;
//    delete listViewDelegate;
    delete previewScene;
    delete ui;
}

void MainWindow::on_actionAbout_Caesium_Image_Compressor_triggered()
{
    auto aboutDialog = new AboutDialog();
    aboutDialog->setAttribute(Qt::WA_DeleteOnClose, true);
    aboutDialog->setWindowFlags(Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    aboutDialog->setWindowModality(Qt::ApplicationModal);
    aboutDialog->show();
}

void MainWindow::on_addFiles_Button_clicked()
{
    return MainWindow::triggerImportFiles();
}

void MainWindow::on_actionAdd_files_triggered()
{
    return MainWindow::triggerImportFiles();
}

void MainWindow::triggerImportFiles()
{
    //Generate file dialog for import and call the progress dialog indicator
    QStringList fileList = QFileDialog::getOpenFileNames(this,
        tr("Import files..."),
        QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0),
        QIODevice::tr("Image Files") + " (*.jpg *.jpeg *.png)");

    return MainWindow::importFiles(fileList);
}

void MainWindow::triggerImportFolder()
{
    QString directoryPath = QFileDialog::getExistingDirectory(this, tr("Import folder..."),
        QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    QStringList fileList = scanDirectory(directoryPath);

    return MainWindow::importFiles(fileList);
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("mainwindow/size", this->size());
    settings.setValue("mainwindow/pos", this->pos());


    settings.setValue("compression_options/compression/level", this->ui->compression_Slider->value());
    settings.setValue("compression_options/compression/lossless", this->ui->lossless_Checkbox->isChecked());
    settings.setValue("compression_options/compression/keep_metadata", this->ui->lossless_Checkbox->isChecked());

    settings.setValue("compression_options/output/output_folder", this->ui->outputFolder_LineEdit->text());
    settings.setValue("compression_options/output/output_suffix", this->ui->outputSuffix_LineEdit->text());
}

void MainWindow::writeSetting(const QString &key, const QVariant &value)
{
    QSettings settings;
    settings.setValue(key, value);
}

void MainWindow::readSettings()
{
    QSettings settings;
    this->resize(settings.value("mainwindow/size").toSize());
    this->move(settings.value("mainwindow/pos").toPoint());


    this->ui->compression_Slider->setValue(settings.value("compression_options/compression/level", 4).toInt());
    this->ui->lossless_Checkbox->setChecked(settings.value("compression_options/compression/lossless").toBool());
    this->ui->keepMetadata_Checkbox->setChecked(settings.value("compression_options/compression/keep_metadata").toBool());

    this->ui->outputFolder_LineEdit->setText(settings.value("compression_options/output/output_folder").toString());
    this->ui->outputSuffix_LineEdit->setText(settings.value("compression_options/output/output_suffix").toString());
}

void MainWindow::previewImage(const QModelIndex &imageIndex)
{
    this->previewScene->clear();
    CImage* cImage = this->cImageModel->getRootItem()->children().at(imageIndex.row())->getCImage();
    QPixmap pixmap(cImage->getCompressedFullPath().isEmpty() ? cImage->getFullPath() : cImage->getCompressedFullPath());
    this->previewScene->addPixmap(pixmap);

    this->previewScene->setSceneRect(this->previewScene->itemsBoundingRect());
    ui->preview_graphicsView->fitInView(this->previewScene->itemsBoundingRect(), Qt::KeepAspectRatio);
    ui->preview_graphicsView->show();
}

QVariant MainWindow::readSetting(const QString &key)
{
    QSettings settings;
    return settings.value(key);
}

void MainWindow::importFiles(const QStringList& fileList)
{
    int listLength = fileList.count();
    QProgressDialog progressDialog(tr("Importing files..."), tr("Cancel"), 0, listLength, this);
    progressDialog.setWindowModality(Qt::WindowModal);

    QList<CImage*> list;
    //TODO use an interator
    for (int i = 0; i < listLength; i++) {
        if (progressDialog.wasCanceled()) {
            break;
        }

        CImage* cImage = new CImage(fileList.at(i));
        if (this->cImageModel->contains(cImage)) {
            break;
        }
        list.append(cImage);

        progressDialog.setValue(i);
    }

    this->cImageModel->appendItems(list);
    progressDialog.setValue(listLength);
}

void MainWindow::on_compress_Button_clicked()
{
    QProgressDialog* progressDialog = new QProgressDialog(tr("Compressing..."), tr("Cancel"), 0, this->cImageModel->getRootItem()->childCount(), this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setCancelButton(nullptr);

    connect(&this->compressionWatcher, SIGNAL(finished()), progressDialog, SLOT(close()));
    connect(&this->compressionWatcher, SIGNAL(finished()), progressDialog, SLOT(deleteLater()));
    connect(&this->compressionWatcher, SIGNAL(finished()), this, SLOT(compressionFinished()));
    connect(&this->compressionWatcher, SIGNAL(progressValueChanged(int)), progressDialog, SLOT(setValue(int)));
    connect(&this->compressionWatcher, SIGNAL(progressValueChanged(int)), this->cImageModel, SLOT(emitDataChanged(int)));
    //    connect(progressDialog, SIGNAL(canceled()), &this->compressionWatcher, SLOT(cancel())); //TODO Does not work like that
    //TODO add future cleanup
    progressDialog->show();

    QFuture<void> future = this->cImageModel->getRootItem()->compress();
    this->compressionWatcher.setFuture(future);
}

void MainWindow::on_actionAdd_folder_triggered()
{
    return MainWindow::triggerImportFolder();
}

void MainWindow::on_removeFiles_Button_clicked()
{
    QModelIndexList indexes = ui->imageList_TreeView->selectionModel()->selectedIndexes();
    std::sort(indexes.begin(), indexes.end(), [](const QModelIndex& a, const QModelIndex& b) {
        return a.row() < b.row();
    });

    int columnCount = this->cImageModel->columnCount();

    for (int i = indexes.count() / columnCount; i > 0; i--) {
        this->cImageModel->removeRows(indexes.at(i).row(), 1, indexes.at(i).parent());
    }
    this->previewScene->clear();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    this->writeSettings();
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    ui->preview_graphicsView->fitInView(this->previewScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void MainWindow::on_outputFolderBrowse_Button_clicked()
{
    QString directoryPath = QFileDialog::getExistingDirectory(this, tr("Select output folder..."),
        QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0),
        QFileDialog::ShowDirsOnly);

    if (!directoryPath.isEmpty()) {
        this->ui->outputFolder_LineEdit->setText(directoryPath);

        this->writeSetting("compression_options/output/output_folder", directoryPath);
    }
}

void MainWindow::on_outputSuffix_LineEdit_textChanged(const QString &arg1)
{
    this->writeSetting("compression_options/output/output_suffix", arg1);
}

void MainWindow::on_lossless_Checkbox_stateChanged(int arg1)
{
    this->writeSetting("compression_options/compression/lossless", arg1 != 0);
}

void MainWindow::on_keepMetadata_Checkbox_stateChanged(int arg1)
{
     this->writeSetting("compression_options/compression/keep_metadata", arg1 != 0);
}

void MainWindow::on_compression_Slider_valueChanged(int value)
{
    this->writeSetting("compression_options/compression/level", value);
}

void MainWindow::imageList_selectionChanged(const QModelIndex &current, const QModelIndex &previous)
{
    if (ui->imageList_TreeView->selectionModel()->selectedRows().count() > 1 || current.row() == previous.row() || current.row() == -1) {
        return;
    }

    this->previewImage(current);
}

void MainWindow::compressionFinished()
{
    if (ui->imageList_TreeView->selectionModel()->selectedRows().count() > 0) {
        this->previewImage(ui->imageList_TreeView->selectionModel()->selectedRows().at(0));
    }
}
