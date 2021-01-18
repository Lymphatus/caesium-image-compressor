#include "MainWindow.h"
#include "./delegates/HtmlDelegate.h"
#include "./exceptions/ImageNotSupportedException.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QStandardPaths>
#include <QtConcurrent>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug() << "Starting UI";

    qRegisterMetaTypeStreamOperators<QList<int>>();

    this->cImageModel = new CImageTreeModel();
    this->previewScene = new QGraphicsScene();
    this->aboutDialog = new AboutDialog(this);

    ui->preview_graphicsView->setScene(this->previewScene);
    ui->imageList_TreeView->setModel(this->cImageModel);
    ui->imageList_TreeView->setIconSize(QSize(10, 10));
    ui->imageList_TreeView->header()->setSectionResizeMode(CImageColumns::NAME, QHeaderView::Stretch);
    ui->imageList_TreeView->header()->setSectionResizeMode(CImageColumns::NAME, QHeaderView::Stretch);
    ui->imageList_TreeView->setItemDelegate(new HtmlDelegate());

    ui->JPEGOptions_GroupBox->setHidden(true);
    ui->PNGOptions_GroupBox->setHidden(true);

    this->initStatusBar();

    connect(ui->imageList_TreeView->selectionModel(), SIGNAL(currentRowChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(imageList_selectionChanged(const QModelIndex&, const QModelIndex&)));
    connect(ui->imageList_TreeView, SIGNAL(dropFinished(QStringList)), this, SLOT(dropFinished(QStringList)));
    connect(this->cImageModel, SIGNAL(itemsChanged()), this, SLOT(cModelItemsChanged()));

    this->readSettings();

    this->on_fitTo_ComboBox_currentIndexChanged(ui->fitTo_ComboBox->currentIndex());
    this->on_keepAspectRatio_CheckBox_toggled(ui->keepAspectRatio_CheckBox->isChecked());
    this->on_doNotEnlarge_CheckBox_toggled(ui->doNotEnlarge_CheckBox->isChecked());
    this->on_keepAspectRatio_CheckBox_toggled(ui->keepAspectRatio_CheckBox->isChecked());
    this->on_sameOutputFolderAsInput_CheckBox_toggled(ui->sameOutputFolderAsInput_CheckBox->isChecked());

//    this->initUpdater();
}

MainWindow::~MainWindow()
{
    if (!this->updaterThread.isFinished()) {
        //Try to exit cleanly
        this->updaterThread.quit();
        this->updaterThread.wait();
    }

    delete cImageModel;
    delete previewScene;
    delete ui;
}

void MainWindow::initStatusBar()
{
    ui->updateAvailable_Button->setHidden(true);
    ui->statusbar->addPermanentWidget(ui->updateAvailable_Button);
}

void MainWindow::on_actionAbout_Caesium_Image_Compressor_triggered()
{
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

    if (fileList.isEmpty()) {
        return;
    }

    QString baseFolder = QFileInfo(fileList.at(0)).absolutePath();

    return MainWindow::importFiles(fileList, baseFolder);
}

void MainWindow::triggerImportFolder()
{
    QString directoryPath = QFileDialog::getExistingDirectory(this, tr("Import folder..."),
        QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    QStringList fileList = scanDirectory(directoryPath);

    if (fileList.isEmpty()) {
        return;
    }

    return MainWindow::importFiles(fileList, directoryPath);
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("mainwindow/size", this->size());
    settings.setValue("mainwindow/pos", this->pos());
    settings.setValue("mainwindow/left_splitter_sizes", QVariant::fromValue<QList<int>>(this->ui->left_Splitter->sizes()));
    settings.setValue("mainwindow/main_splitter_sizes", QVariant::fromValue<QList<int>>(this->ui->main_Splitter->sizes()));

    settings.setValue("compression_options/compression/level", this->ui->compression_Slider->value());
    settings.setValue("compression_options/compression/lossless", this->ui->lossless_Checkbox->isChecked());
    settings.setValue("compression_options/compression/keep_metadata", this->ui->keepMetadata_Checkbox->isChecked());
    settings.setValue("compression_options/compression/keep_structure", this->ui->keepStructure_Checkbox->isChecked());
    settings.setValue("compression_options/compression/advanced_mode", this->ui->advancedMode_Button->isChecked());
    settings.setValue("compression_options/compression/jpeg_quality", this->ui->JPEGQuality_Slider->value());
    settings.setValue("compression_options/compression/png_iterations", this->ui->PNGIterations_SpinBox->value());
    settings.setValue("compression_options/compression/png_iterations_large", this->ui->PNGIterationsLarge_SpinBox->value());
    settings.setValue("compression_options/compression/png_lossy8", this->ui->PNGLossy8_CheckBox->isChecked());
    settings.setValue("compression_options/compression/png_transparent", this->ui->PNGLossyTransparent_CheckBox->isChecked());

    settings.setValue("compression_options/resize/resize", this->ui->resize_groupBox->isChecked());
    settings.setValue("compression_options/resize/fit_to", this->ui->fitTo_ComboBox->currentIndex());
    settings.setValue("compression_options/resize/width", this->ui->width_SpinBox->value());
    settings.setValue("compression_options/resize/height", this->ui->height_SpinBox->value());
    settings.setValue("compression_options/resize/size", this->ui->edge_SpinBox->value());
    settings.setValue("compression_options/resize/keep_aspect_ratio", this->ui->keepAspectRatio_CheckBox->isChecked());
    settings.setValue("compression_options/resize/do_not_enlarge", this->ui->doNotEnlarge_CheckBox->isChecked());

    settings.setValue("compression_options/output/output_folder", this->ui->outputFolder_LineEdit->text());
    settings.setValue("compression_options/output/output_suffix", this->ui->outputSuffix_LineEdit->text());
    settings.setValue("compression_options/output/same_folder_as_input", this->ui->sameOutputFolderAsInput_CheckBox->isChecked());
}

void MainWindow::writeSetting(const QString& key, const QVariant& value)
{
    QSettings settings;
    settings.setValue(key, value);
}

void MainWindow::readSettings()
{
    QSettings settings;
    this->resize(settings.value("mainwindow/size").toSize());
    this->move(settings.value("mainwindow/pos").toPoint());

    this->ui->left_Splitter->setSizes(settings.value("mainwindow/left_splitter_sizes", QVariant::fromValue<QList<int>>({ 100, 1 })).value<QList<int>>());
    this->ui->main_Splitter->setSizes(settings.value("mainwindow/main_splitter_sizes", QVariant::fromValue<QList<int>>({ 700, 1 })).value<QList<int>>());

    this->ui->compression_Slider->setValue(settings.value("compression_options/compression/level", 4).toInt());
    this->ui->lossless_Checkbox->setChecked(settings.value("compression_options/compression/lossless", false).toBool());
    this->ui->keepMetadata_Checkbox->setChecked(settings.value("compression_options/compression/keep_metadata", true).toBool());
    this->ui->keepStructure_Checkbox->setChecked(settings.value("compression_options/compression/keep_structure", false).toBool());
    this->ui->advancedMode_Button->setChecked(settings.value("compression_options/compression/advanced_mode", false).toBool());
    this->ui->JPEGQuality_Slider->setValue(settings.value("compression_options/compression/jpeg_quality", 80).toInt());
    this->ui->JPEGQuality_SpinBox->setValue(settings.value("compression_options/compression/jpeg_quality", 80).toInt());
    this->ui->PNGIterations_SpinBox->setValue(settings.value("compression_options/compression/png_iterations", 5).toInt());
    this->ui->PNGIterationsLarge_SpinBox->setValue(settings.value("compression_options/compression/png_iterations_large", 2).toInt());
    this->ui->PNGLossy8_CheckBox->setChecked(settings.value("compression_options/compression/png_lossy8", false).toBool());
    this->ui->PNGLossyTransparent_CheckBox->setChecked(settings.value("compression_options/compression/png_transparent", false).toInt());

    this->ui->resize_groupBox->setChecked(settings.value("compression_options/resize/resize", false).toBool());
    this->ui->fitTo_ComboBox->setCurrentIndex(settings.value("compression_options/resize/fit_to", 0).toInt());
    this->ui->width_SpinBox->setValue(settings.value("compression_options/resize/width", 1000).toInt());
    this->ui->height_SpinBox->setValue(settings.value("compression_options/resize/height", 1000).toInt());
    this->ui->edge_SpinBox->setValue(settings.value("compression_options/resize/size", 1000).toInt());
    this->ui->keepAspectRatio_CheckBox->setChecked(settings.value("compression_options/resize/keep_aspect_ratio", false).toBool());
    this->ui->doNotEnlarge_CheckBox->setChecked(settings.value("compression_options/resize/do_not_enlarge", false).toBool());

    this->ui->outputFolder_LineEdit->setText(settings.value("compression_options/output/output_folder", "").toString());
    this->ui->outputSuffix_LineEdit->setText(settings.value("compression_options/output/output_suffix", "").toString());
    this->ui->sameOutputFolderAsInput_CheckBox->setChecked(settings.value("compression_options/output/same_folder_as_input", false).toBool());
}

void MainWindow::previewImage(const QModelIndex& imageIndex)
{
    this->previewScene->clear();
    CImage* cImage = this->cImageModel->getRootItem()->children().at(imageIndex.row())->getCImage();
    QPixmap pixmap(cImage->getCompressedFullPath().isEmpty() ? cImage->getFullPath() : cImage->getCompressedFullPath());
    this->previewScene->addPixmap(pixmap);

    this->previewScene->setSceneRect(this->previewScene->itemsBoundingRect());
    ui->preview_graphicsView->fitInView(this->previewScene->itemsBoundingRect(), Qt::KeepAspectRatio);
    ui->preview_graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    ui->preview_graphicsView->show();
}

void MainWindow::updateFolderMap(QString baseFolder, int count)
{
    QStringListIterator it(this->folderMap.keys());
    bool isAlreadyInList = false;
    while (it.hasNext() && !isAlreadyInList) {
        QString folderInMap = it.next();
        isAlreadyInList = baseFolder.startsWith(folderInMap);
        if (isAlreadyInList) {
            baseFolder = folderInMap;
        }
    }

    if (!isAlreadyInList) {
        folderMap.insert(baseFolder, count);
    } else {
        folderMap[baseFolder] += count;
    }

    if (folderMap[baseFolder] == 0) {
        folderMap.remove(baseFolder);
    }
}

QVariant MainWindow::readSetting(const QString& key)
{
    QSettings settings;
    return settings.value(key);
}

void MainWindow::importFiles(const QStringList& fileList, QString baseFolder)
{
    int listLength = fileList.count();
    QProgressDialog progressDialog(tr("Importing files..."), tr("Cancel"), 0, listLength, this);
    progressDialog.setWindowModality(Qt::WindowModal);

    QList<CImage*> list;
    //TODO use an iterator
    for (int i = 0; i < listLength; i++) {
        if (progressDialog.wasCanceled()) {
            break;
        }

        try {
            auto* cImage = new CImage(fileList.at(i));
            if (this->cImageModel->contains(cImage)) {
                break;
            }
            list.append(cImage);
        } catch (ImageNotSupportedException& e) {
            qWarning() << fileList.at(i) << "is not supported";
        }

        progressDialog.setValue(i);
    }

    if (!list.isEmpty()) {
        this->updateFolderMap(baseFolder, list.count());
        this->cImageModel->appendItems(list);
    }

    progressDialog.setValue(listLength);
}

void MainWindow::removeFiles(bool all)
{
    if (all) {
        ui->imageList_TreeView->selectAll();
    }
    QModelIndexList indexes = ui->imageList_TreeView->selectionModel()->selectedIndexes();
    std::sort(indexes.begin(), indexes.end(), [](const QModelIndex& a, const QModelIndex& b) {
        return a.row() < b.row();
    });

    int columnCount = this->cImageModel->columnCount();

    for (int i = indexes.count() / columnCount; i > 0; i--) {
        this->updateFolderMap(this->cImageModel->getRootItem()->children().at(indexes.at(i).row())->getCImage()->getFullPath(), -1);
        this->cImageModel->removeRows(indexes.at(i).row(), 1, indexes.at(i).parent());
    }
    this->previewScene->clear();
    this->previewScene->setSceneRect(this->previewScene->itemsBoundingRect());
    ui->preview_graphicsView->fitInView(this->previewScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void MainWindow::on_compress_Button_clicked()
{
    QSettings settings;

    if (settings.value("compression_options/output/output_folder").toString().isEmpty()) {
        QMessageBox msgBox;
        msgBox.setText("Please select an output folder first");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        return;
    }
    if (this->cImageModel->getRootItem()->childCount() == 0) {
        return;
    }
    QProgressDialog* progressDialog = new QProgressDialog(tr("Compressing..."), tr("Cancel"), 0, this->cImageModel->getRootItem()->childCount(), this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setCancelButton(nullptr);

    connect(&this->compressionWatcher, SIGNAL(finished()), progressDialog, SLOT(close()));
    connect(&this->compressionWatcher, SIGNAL(finished()), progressDialog, SLOT(deleteLater()));
    connect(&this->compressionWatcher, SIGNAL(finished()), this, SLOT(compressionFinished()));
    connect(&this->compressionWatcher, SIGNAL(progressValueChanged(int)), progressDialog, SLOT(setValue(int)));
    connect(&this->compressionWatcher, SIGNAL(progressValueChanged(int)), this->cImageModel, SLOT(emitDataChanged(int)));
    //TODO add future cleanup
    progressDialog->show();

    cs_jpeg_pars advancedJPEGPars;
    cs_png_pars advancedPNGPars;

    advancedJPEGPars.quality = qBound(this->ui->JPEGQuality_Slider->value(), 1, 100);
    advancedJPEGPars.exif_copy = this->ui->keepMetadata_Checkbox->isChecked();

    advancedPNGPars.iterations = this->ui->PNGIterations_SpinBox->value();
    advancedPNGPars.iterations_large = this->ui->PNGIterationsLarge_SpinBox->value();
    advancedPNGPars.lossy_8 = this->ui->PNGLossy8_CheckBox->isChecked();
    advancedPNGPars.transparent = this->ui->PNGLossyTransparent_CheckBox->isChecked();

    CompressionOptions compressionOptions = {
        this->ui->outputFolder_LineEdit->text(),
        getRootFolder(this->folderMap),
        this->ui->outputSuffix_LineEdit->text(),
        this->ui->compression_Slider->value(),
        this->ui->lossless_Checkbox->isChecked(),
        this->ui->keepMetadata_Checkbox->isChecked(),
        this->ui->keepStructure_Checkbox->isChecked(),
        this->ui->resize_groupBox->isChecked(),
        this->ui->fitTo_ComboBox->currentIndex(),
        this->ui->width_SpinBox->value(),
        this->ui->height_SpinBox->value(),
        this->ui->edge_SpinBox->value(),
        this->ui->doNotEnlarge_CheckBox->isChecked(),
        this->ui->sameOutputFolderAsInput_CheckBox->isChecked(),
        this->ui->advancedMode_Button->isChecked(),
        advancedJPEGPars,
        advancedPNGPars
    };

    QFuture<void> future = this->cImageModel->getRootItem()->compress(compressionOptions);
    this->compressionWatcher.setFuture(future);
}

void MainWindow::on_actionAdd_folder_triggered()
{
    return MainWindow::triggerImportFolder();
}

void MainWindow::on_removeFiles_Button_clicked()
{
    this->removeFiles(false);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    this->writeSettings();
    qInfo() << "---- Closing application ----";
    event->accept();
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

void MainWindow::on_outputSuffix_LineEdit_textChanged(const QString& arg1)
{
    this->writeSetting("compression_options/output/output_suffix", arg1);
}

void MainWindow::on_compression_Slider_valueChanged(int value)
{
    this->writeSetting("compression_options/compression/level", value);
}

void MainWindow::imageList_selectionChanged(const QModelIndex& current, const QModelIndex& previous)
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

void MainWindow::on_actionRemove_triggered()
{
    this->removeFiles(false);
}

void MainWindow::on_actionClear_triggered()
{
    this->removeFiles(true);
}
void MainWindow::dropFinished(QStringList filePaths)
{
    QString baseFolder = QFileInfo(filePaths.at(0)).absolutePath();
    MainWindow::importFiles(filePaths, baseFolder);
}

void MainWindow::on_fitTo_ComboBox_currentIndexChanged(int index)
{
    QSettings settings;
    switch (index) {
    default:
    case ResizeMode::DIMENSIONS:
        ui->edge_Label->hide();
        ui->edge_SpinBox->hide();
        ui->width_Label->show();
        ui->width_SpinBox->show();
        ui->width_SpinBox->setSuffix(tr("px"));
        ui->width_SpinBox->setMaximum(99999);
        ui->height_Label->show();
        ui->height_SpinBox->show();
        ui->height_SpinBox->setSuffix(tr("px"));
        ui->height_SpinBox->setMaximum(99999);
        ui->keepAspectRatio_CheckBox->setDisabled(true);
        break;
    case ResizeMode::PERCENTAGE:
        ui->edge_Label->hide();
        ui->edge_SpinBox->hide();
        ui->width_Label->show();
        ui->width_SpinBox->show();
        ui->width_SpinBox->setSuffix(tr("%"));
        ui->width_SpinBox->setMaximum(ui->keepAspectRatio_CheckBox->isChecked() ? 100 : 999);
        ui->height_Label->show();
        ui->height_SpinBox->show();
        ui->height_SpinBox->setSuffix(tr("%"));
        ui->height_SpinBox->setMaximum(ui->keepAspectRatio_CheckBox->isChecked() ? 100 : 999);
        ui->keepAspectRatio_CheckBox->setEnabled(true);
        break;
    case ResizeMode::SHORT_EDGE:
    case ResizeMode::LONG_EDGE:
        ui->edge_Label->show();
        ui->edge_SpinBox->show();
        ui->width_Label->hide();
        ui->width_SpinBox->hide();
        ui->height_Label->hide();
        ui->height_SpinBox->hide();
        ui->keepAspectRatio_CheckBox->setDisabled(true);
        break;
    }

    this->writeSetting("compression_options/resize/fit_to", index);
}

void MainWindow::on_resize_groupBox_toggled(bool checked)
{
    this->writeSetting("compression_options/resize/resize", checked);
}

void MainWindow::on_width_SpinBox_valueChanged(int value)
{
    if (this->ui->fitTo_ComboBox->currentIndex() == ResizeMode::PERCENTAGE && this->ui->keepAspectRatio_CheckBox->isChecked()) {
        this->ui->height_SpinBox->setValue(value);
    }
    this->writeSetting("compression_options/resize/width", value);
    this->writeSetting("compression_options/resize/height", this->ui->height_SpinBox->value());
}

void MainWindow::on_height_SpinBox_valueChanged(int value)
{
    if (this->ui->fitTo_ComboBox->currentIndex() == ResizeMode::PERCENTAGE && this->ui->keepAspectRatio_CheckBox->isChecked()) {
        this->ui->width_SpinBox->setValue(value);
    }
    this->writeSetting("compression_options/resize/height", value);
    this->writeSetting("compression_options/resize/width", this->ui->width_SpinBox->value());
}

void MainWindow::on_edge_SpinBox_valueChanged(int value)
{
    this->writeSetting("compression_options/resize/size", value);
}

void MainWindow::on_keepAspectRatio_CheckBox_toggled(bool checked)
{
    if (this->ui->fitTo_ComboBox->currentIndex() == ResizeMode::PERCENTAGE && checked) {
        this->ui->height_SpinBox->setValue(this->ui->width_SpinBox->value());
    }
    this->writeSetting("compression_options/resize/keep_aspect_ratio", checked);
}

void MainWindow::on_doNotEnlarge_CheckBox_toggled(bool checked)
{
    if (this->ui->fitTo_ComboBox->currentIndex() == ResizeMode::PERCENTAGE && checked) {
        this->ui->width_SpinBox->setMaximum(100);
        this->ui->height_SpinBox->setMaximum(100);
    } else {
        int maximum = this->ui->fitTo_ComboBox->currentIndex() == ResizeMode::PERCENTAGE ? 999 : 99999;
        this->ui->width_SpinBox->setMaximum(maximum);
        this->ui->height_SpinBox->setMaximum(maximum);
    }
    this->writeSetting("compression_options/resize/do_not_enlarge", checked);
}

void MainWindow::on_actionSelect_All_triggered()
{
    this->ui->imageList_TreeView->selectAll();
}

void MainWindow::on_sameOutputFolderAsInput_CheckBox_toggled(bool checked)
{
    this->ui->keepStructure_Checkbox->setEnabled(checked);
    if (!checked) {
        this->ui->keepStructure_Checkbox->setChecked(false);
    }
    this->writeSetting("compression_options/output/same_folder_as_input", checked);
}

void MainWindow::on_keepStructure_Checkbox_toggled(bool checked)
{
    this->writeSetting("compression_options/compression/keep_structure", checked);
}

void MainWindow::on_lossless_Checkbox_toggled(bool checked)
{
    this->writeSetting("compression_options/compression/lossless", checked);
}

void MainWindow::on_keepMetadata_Checkbox_toggled(bool checked)
{
    this->writeSetting("compression_options/compression/keep_metadata", checked);
}

void MainWindow::on_advancedMode_Button_toggled(bool checked)
{
    QGridLayout* compressionLayout = (QGridLayout*)this->ui->compressionOptions_GroupBox->layout();
    QGridLayout* jpegLayout = (QGridLayout*)this->ui->JPEGOptions_GroupBox->layout();
    if (checked) {
        compressionLayout->removeWidget(this->ui->lossless_Checkbox);
        compressionLayout->removeWidget(this->ui->keepMetadata_Checkbox);

        jpegLayout->addWidget(this->ui->lossless_Checkbox, 2, 0);
        jpegLayout->addWidget(this->ui->keepMetadata_Checkbox, 3, 0);
    } else {
        jpegLayout->removeWidget(this->ui->lossless_Checkbox);
        jpegLayout->removeWidget(this->ui->keepMetadata_Checkbox);
        jpegLayout->removeWidget(this->ui->advancedMode_Button);

        compressionLayout->addWidget(this->ui->lossless_Checkbox);
        compressionLayout->addWidget(this->ui->keepMetadata_Checkbox);
        compressionLayout->addWidget(this->ui->advancedMode_Button);
    }

    this->writeSetting("compression_options/compression/advanced_mode", checked);
}

void MainWindow::on_JPEGQuality_Slider_valueChanged(int value)
{
    this->writeSetting("compression_options/compression/jpeg_quality", value);
}

void MainWindow::on_JPEGQuality_SpinBox_valueChanged(int value)
{
    this->writeSetting("compression_options/compression/jpeg_quality", value);
}

void MainWindow::on_PNGIterations_SpinBox_valueChanged(int value)
{
    this->writeSetting("compression_options/compression/png_iterations", value);
}

void MainWindow::on_PNGIterationsLarge_SpinBox_valueChanged(int value)
{
    this->writeSetting("compression_options/compression/png_iterations_large", value);
}

void MainWindow::on_PNGLossy8_CheckBox_toggled(bool checked)
{
    this->writeSetting("compression_options/compression/png_lossy8", checked);
}

void MainWindow::on_PNGLossyTransparent_CheckBox_toggled(bool checked)
{
    this->writeSetting("compression_options/compression/png_transparent", checked);
}

void MainWindow::cModelItemsChanged()
{
    int itemsCount = this->cImageModel->rowCount();
    QString humanItemsCount = QString::number(this->cImageModel->rowCount());
    QString totalSize = toHumanSize(this->cImageModel->originalItemsSize());
    ui->statusbar->showMessage(humanItemsCount + " " + tr("images in list") + " | " + totalSize);

    ui->removeFiles_Button->setDisabled(itemsCount == 0);
    ui->actionRemove->setDisabled(itemsCount == 0);
    ui->actionClear->setDisabled(itemsCount == 0);
    ui->actionSelect_All->setDisabled(itemsCount == 0);
}

void MainWindow::on_updateAvailable_Button_clicked()
{
    this->runUpdate();
}

void MainWindow::updateAvailable(const QString &filePath)
{
    this->ui->updateAvailable_Button->setVisible(true);
    this->aboutDialog->updateIsAvailable(filePath);
    this->updateFilePath = filePath;
}

void MainWindow::initUpdater()
{
    auto *updater = new Updater();
    updater->moveToThread(&updaterThread);

    connect(updater, &Updater::finished, aboutDialog, &AboutDialog::checkForUpdatesFinished);
    connect(&updaterThread, &QThread::started, aboutDialog, &AboutDialog::checkForUpdatesStarted);
    connect(&updaterThread, &QThread::finished, updater, &QObject::deleteLater);
    connect(updater, &Updater::resultReady, this, &MainWindow::updateAvailable);
    connect(updater, &Updater::resultReady, aboutDialog, &AboutDialog::updateIsAvailable);

    updaterThread.start();
    updater->checkForUpdates();
}

void MainWindow::runUpdate()
{
    QString currentProcessPath = QCoreApplication::applicationFilePath();
    Updater::replaceCurrentFiles(this->updateFilePath);
    QProcess::startDetached(currentProcessPath, QCoreApplication::arguments());
    QCoreApplication::quit();
}
