#include "MainWindow.h"
#include "./delegates/HtmlDelegate.h"
#include "./exceptions/ImageNotSupportedException.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QObject>
#include <QProgressDialog>
#include <QScrollBar>
#include <QStandardPaths>
#include <QWheelEvent>
#include <QWindow>
#include <QtConcurrent>
#include <dialogs/PreferencesDialog.h>
#include <widgets/QCaesiumMessageBox.h>

#ifdef Q_OS_MAC
#include "./updater/osx/CocoaInitializer.h"
#include "./updater/osx/SparkleAutoUpdater.h"
#endif

#ifdef Q_OS_WIN
#include "./updater/win/winsparkle.h"
#endif

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qDebug() << "Starting UI";

    this->cImageModel = new CImageTreeModel();
    this->previewScene = new QGraphicsScene();
    this->compressedPreviewScene = new QGraphicsScene();
    this->aboutDialog = new AboutDialog(this);
    this->compressionWatcher = new QFutureWatcher<void>();
    this->listContextMenu = new QMenu();

    ui->preview_GraphicsView->setScene(this->previewScene);
    ui->previewCompressed_GraphicsView->setScene(this->compressedPreviewScene);

    ui->imageList_TreeView->setModel(this->cImageModel);
    ui->imageList_TreeView->setIconSize(QSize(10, 10));
    ui->imageList_TreeView->header()->setSectionResizeMode(CImageColumns::NAME, QHeaderView::Stretch);
    ui->imageList_TreeView->header()->setSectionResizeMode(CImageColumns::NAME, QHeaderView::Stretch);
    ui->imageList_TreeView->setItemDelegate(new HtmlDelegate());

    ui->edge_Label->hide();
    ui->edge_SpinBox->hide();

    this->keepDatesButtonGroup = new QButtonGroup();
    this->keepDatesButtonGroup->setExclusive(false);
    this->keepDatesButtonGroup->addButton(ui->keepCreationDate_CheckBox);
    this->keepDatesButtonGroup->addButton(ui->keepLastModifiedDate_CheckBox);
    this->keepDatesButtonGroup->addButton(ui->keepLastAccessDate_CheckBox);

    this->initStatusBar();
    this->initListContextMenu();

    QObject::connect(ui->imageList_TreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::imageList_selectionChanged);
    connect(ui->imageList_TreeView, SIGNAL(dropFinished(QStringList)), this, SLOT(dropFinished(QStringList)));
    connect(this->cImageModel, SIGNAL(itemsChanged()), this, SLOT(cModelItemsChanged()));
    connect(ui->preview_GraphicsView, SIGNAL(scaleFactorChanged(QWheelEvent*)), ui->previewCompressed_GraphicsView, SLOT(setScaleFactor(QWheelEvent*)));
    connect(ui->previewCompressed_GraphicsView, SIGNAL(scaleFactorChanged(QWheelEvent*)), ui->preview_GraphicsView, SLOT(setScaleFactor(QWheelEvent*)));
    connect(ui->imageList_TreeView, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showListContextMenu(const QPoint&)));

    connect(ui->preview_GraphicsView->horizontalScrollBar(), SIGNAL(valueChanged(int)), ui->previewCompressed_GraphicsView, SLOT(setHorizontalScrollBarValue(int)));
    connect(ui->preview_GraphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->previewCompressed_GraphicsView, SLOT(setVerticalScrollBarValue(int)));
    connect(ui->previewCompressed_GraphicsView->horizontalScrollBar(), SIGNAL(valueChanged(int)), ui->preview_GraphicsView, SLOT(setHorizontalScrollBarValue(int)));
    connect(ui->previewCompressed_GraphicsView->verticalScrollBar(), SIGNAL(valueChanged(int)), ui->preview_GraphicsView, SLOT(setVerticalScrollBarValue(int)));
    QObject::connect(keepDatesButtonGroup, &QButtonGroup::buttonClicked, this, &MainWindow::keepDatesButtonGroupClicked);

    this->readSettings();

    this->on_fitTo_ComboBox_currentIndexChanged(ui->fitTo_ComboBox->currentIndex());
    this->on_keepAspectRatio_CheckBox_toggled(ui->keepAspectRatio_CheckBox->isChecked());
    this->on_doNotEnlarge_CheckBox_toggled(ui->doNotEnlarge_CheckBox->isChecked());
    this->on_keepAspectRatio_CheckBox_toggled(ui->keepAspectRatio_CheckBox->isChecked());
    this->on_sameOutputFolderAsInput_CheckBox_toggled(ui->sameOutputFolderAsInput_CheckBox->isChecked());
}

MainWindow::~MainWindow()
{
#ifdef Q_OS_WIN
    win_sparkle_cleanup();
#endif

    delete cImageModel;
    delete previewScene;
    delete compressedPreviewScene;
    delete aboutDialog;
    delete keepDatesButtonGroup;
    delete compressionWatcher;
    delete ui;
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    this->initUpdater();
}

void MainWindow::initStatusBar()
{
    ui->statusbar->addPermanentWidget(ui->version_Label);
    ui->version_Label->setText(QCoreApplication::applicationVersion());
}

void MainWindow::initListContextMenu()
{
    this->listContextMenu->addAction(ui->actionAdd_files);
    this->listContextMenu->addAction(ui->actionAdd_folder);
    this->listContextMenu->addSeparator();
    this->listContextMenu->addAction(ui->actionRemove);
    this->listContextMenu->addAction(ui->actionClear);
}

void MainWindow::on_actionAbout_Caesium_Image_Compressor_triggered()
{
    aboutDialog->setWindowModality(Qt::NonModal);
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
    // Generate file dialog for import and call the progress dialog indicator
    QStringList fileList = QFileDialog::getOpenFileNames(this,
        tr("Import files..."),
        this->lastOpenedDirectory,
        QIODevice::tr("Image Files") + " (*.jpg *.jpeg *.png *.webp)");

    if (fileList.isEmpty()) {
        return;
    }

    QString baseFolder = QFileInfo(fileList.at(0)).absolutePath();
    this->lastOpenedDirectory = baseFolder;

    return MainWindow::importFiles(fileList, baseFolder);
}

void MainWindow::triggerImportFolder()
{
    QString directoryPath = QFileDialog::getExistingDirectory(this, tr("Import folder..."),
        this->lastOpenedDirectory,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (directoryPath.isEmpty()) {
        return;
    }

    QStringList fileList = scanDirectory(directoryPath);

    if (fileList.isEmpty()) {
        return;
    }

    this->lastOpenedDirectory = directoryPath;
    return MainWindow::importFiles(fileList, directoryPath);
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("mainwindow/maximized", this->isMaximized());
    settings.setValue("mainwindow/size", this->size());
    settings.setValue("mainwindow/pos", this->pos());
    settings.setValue("mainwindow/left_splitter_sizes", QVariant::fromValue<QList<int>>(this->ui->sidebar_HSplitter->sizes()));
    settings.setValue("mainwindow/previews_visible", ui->actionShow_previews->isChecked());
    if (ui->actionShow_previews->isChecked()) {
        settings.setValue("mainwindow/main_splitter_sizes", QVariant::fromValue<QList<int>>(this->ui->main_VSplitter->sizes()));
    }

    settings.setValue("compression_options/compression/lossless", this->ui->lossless_CheckBox->isChecked());
    settings.setValue("compression_options/compression/keep_metadata", this->ui->keepMetadata_CheckBox->isChecked());
    settings.setValue("compression_options/compression/keep_structure", this->ui->keepStructure_CheckBox->isChecked());
    settings.setValue("compression_options/compression/jpeg_quality", this->ui->JPEGQuality_Slider->value());
    settings.setValue("compression_options/compression/png_level", this->ui->PNGLevel_Slider->value());
    settings.setValue("compression_options/compression/webp_quality", this->ui->WebPQuality_Slider->value());

    settings.setValue("compression_options/resize/resize", this->ui->fitTo_ComboBox->currentIndex() != ResizeMode::NO_RESIZE);
    settings.setValue("compression_options/resize/fit_to", this->ui->fitTo_ComboBox->currentIndex());
    settings.setValue("compression_options/resize/width", this->ui->width_SpinBox->value());
    settings.setValue("compression_options/resize/height", this->ui->height_SpinBox->value());
    settings.setValue("compression_options/resize/size", this->ui->edge_SpinBox->value());
    settings.setValue("compression_options/resize/keep_aspect_ratio", this->ui->keepAspectRatio_CheckBox->isChecked());
    settings.setValue("compression_options/resize/do_not_enlarge", this->ui->doNotEnlarge_CheckBox->isChecked());

    settings.setValue("compression_options/output/output_folder", this->ui->outputFolder_LineEdit->text());
    settings.setValue("compression_options/output/output_suffix", this->ui->outputSuffix_LineEdit->text());
    settings.setValue("compression_options/output/same_folder_as_input", this->ui->sameOutputFolderAsInput_CheckBox->isChecked());
    settings.setValue("compression_options/output/keep_dates", this->ui->keepDates_CheckBox->checkState());
    settings.setValue("compression_options/output/keep_creation_date", ui->keepCreationDate_CheckBox->isChecked());
    settings.setValue("compression_options/output/keep_last_modified_date", ui->keepLastModifiedDate_CheckBox->isChecked());
    settings.setValue("compression_options/output/keep_last_access_date", ui->keepLastAccessDate_CheckBox->isChecked());

    settings.setValue("extra/last_opened_directory", this->lastOpenedDirectory);
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
    if (settings.value("mainwindow/maximized", false).toBool()) {
        this->showMaximized();
    }

    this->ui->sidebar_HSplitter->setSizes(settings.value("mainwindow/left_splitter_sizes", QVariant::fromValue<QList<int>>({ 600, 1 })).value<QList<int>>());
    this->ui->main_VSplitter->setSizes(settings.value("mainwindow/main_splitter_sizes", QVariant::fromValue<QList<int>>({ 500, 250 })).value<QList<int>>());
    this->ui->actionShow_previews->setChecked(settings.value("mainwindow/previews_visible", true).toBool());

    this->ui->lossless_CheckBox->setChecked(settings.value("compression_options/compression/lossless", false).toBool());
    this->ui->keepMetadata_CheckBox->setChecked(settings.value("compression_options/compression/keep_metadata", true).toBool());
    this->ui->keepStructure_CheckBox->setChecked(settings.value("compression_options/compression/keep_structure", false).toBool());
    this->ui->JPEGQuality_Slider->setValue(settings.value("compression_options/compression/jpeg_quality", 80).toInt());
    this->ui->PNGLevel_SpinBox->setValue(settings.value("compression_options/compression/png_level", 3).toInt());
    this->ui->JPEGQuality_SpinBox->setValue(settings.value("compression_options/compression/jpeg_quality", 80).toInt());
    this->ui->WebPQuality_SpinBox->setValue(settings.value("compression_options/compression/webp_quality", 60).toInt());

    this->ui->fitTo_ComboBox->setCurrentIndex(settings.value("compression_options/resize/fit_to", 0).toInt());
    this->ui->width_SpinBox->setValue(settings.value("compression_options/resize/width", 1000).toInt());
    this->ui->height_SpinBox->setValue(settings.value("compression_options/resize/height", 1000).toInt());
    this->ui->edge_SpinBox->setValue(settings.value("compression_options/resize/size", 1000).toInt());
    this->ui->keepAspectRatio_CheckBox->setChecked(settings.value("compression_options/resize/keep_aspect_ratio", false).toBool());
    this->ui->doNotEnlarge_CheckBox->setChecked(settings.value("compression_options/resize/do_not_enlarge", false).toBool());

    this->ui->outputFolder_LineEdit->setText(settings.value("compression_options/output/output_folder", "").toString());
    this->ui->outputSuffix_LineEdit->setText(settings.value("compression_options/output/output_suffix", "").toString());
    this->ui->sameOutputFolderAsInput_CheckBox->setChecked(settings.value("compression_options/output/same_folder_as_input", false).toBool());
    this->ui->keepDates_CheckBox->setCheckState(settings.value("compression_options/output/keep_dates", Qt::Unchecked).value<Qt::CheckState>());
    this->ui->keepCreationDate_CheckBox->setChecked(settings.value("compression_options/output/keep_creation_date", false).toBool());
    this->ui->keepLastModifiedDate_CheckBox->setChecked(settings.value("compression_options/output/keep_last_modified_date", false).toBool());
    this->ui->keepLastAccessDate_CheckBox->setChecked(settings.value("compression_options/output/keep_last_access_date", false).toBool());

    this->lastOpenedDirectory = settings.value("extra/last_opened_directory", QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0)).toString();
}

void MainWindow::previewImage(const QModelIndex& imageIndex)
{
    QSettings settings;
    if (!settings.value("mainwindow/previews_visible", false).toBool()) {
        return;
    }
    this->previewScene->clear();
    this->compressedPreviewScene->clear();

    ui->preview_GraphicsView->resetScaleFactor();
    ui->previewCompressed_GraphicsView->resetScaleFactor();

    CImage* cImage = this->cImageModel->getRootItem()->children().at(imageIndex.row())->getCImage();
    QPixmap pixmap(cImage->getFullPath());
    this->previewScene->addPixmap(pixmap);

    this->previewScene->setSceneRect(this->previewScene->itemsBoundingRect());
    ui->preview_GraphicsView->fitInView(this->previewScene->itemsBoundingRect(), Qt::KeepAspectRatio);
    ui->preview_GraphicsView->show();

    if (cImage->getStatus() == CImageStatus::COMPRESSED) {
        QPixmap pixmapCompressed(cImage->getCompressedFullPath());
        this->compressedPreviewScene->addPixmap(pixmapCompressed);

        this->compressedPreviewScene->setSceneRect(this->compressedPreviewScene->itemsBoundingRect());
        ui->previewCompressed_GraphicsView->fitInView(this->compressedPreviewScene->itemsBoundingRect(), Qt::KeepAspectRatio);
        ui->previewCompressed_GraphicsView->show();
    } else {
        this->compressedPreviewScene->setSceneRect(this->previewScene->itemsBoundingRect());
        ui->previewCompressed_GraphicsView->fitInView(this->previewScene->itemsBoundingRect(), Qt::KeepAspectRatio);
        ui->previewCompressed_GraphicsView->show();
    }
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
        this->folderMap.insert(baseFolder, count);
    } else {
        this->folderMap[baseFolder] += count;
    }

    if (this->folderMap[baseFolder] == 0) {
        this->folderMap.remove(baseFolder);
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
    // TODO use an iterator
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

    if (!list.isEmpty() && listLength > 0) {
        this->updateFolderMap(baseFolder, list.count());
        this->cImageModel->appendItems(list);
        this->importedFilesRootFolder = getRootFolder(this->folderMap);
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
    this->compressedPreviewScene->clear();
    this->previewScene->setSceneRect(this->previewScene->itemsBoundingRect());
    this->previewScene->setSceneRect(this->compressedPreviewScene->itemsBoundingRect());
    ui->preview_GraphicsView->fitInView(this->previewScene->itemsBoundingRect(), Qt::KeepAspectRatio);
}

void MainWindow::on_compress_Button_clicked()
{
    QSettings settings;

    if (this->ui->outputFolder_LineEdit->text().isEmpty() && !this->ui->sameOutputFolderAsInput_CheckBox->isChecked()) {
        QCaesiumMessageBox msgBox;
        msgBox.setText("Please select an output folder first");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        return;
    }

    QString rootFolder = getRootFolder(this->folderMap);

    bool overwriteWarningFlag = (this->ui->sameOutputFolderAsInput_CheckBox->isChecked() && this->ui->outputSuffix_LineEdit->text().isEmpty())
        || rootFolder == this->ui->outputFolder_LineEdit->text();

    if (overwriteWarningFlag) {
        QCaesiumMessageBox sameFolderPrompt;
        sameFolderPrompt.setText(tr("You are about to overwrite your original images and this action can't be undone.\n\nDo you really want to continue?"));
        sameFolderPrompt.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        sameFolderPrompt.setButtonText(QMessageBox::Yes, tr("Yes"));
        sameFolderPrompt.setButtonText(QMessageBox::No, tr("No"));

        int answer = sameFolderPrompt.exec();

        if (answer == QMessageBox::No) {
            return;
        }
    }

    if (this->cImageModel->getRootItem()->childCount() == 0) {
        return;
    }
    auto* progressDialog = new QProgressDialog(tr("Compressing..."), tr("Cancel"), 0, this->cImageModel->getRootItem()->childCount(), this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setCancelButton(nullptr);

    this->compressionWatcher = new QFutureWatcher<void>();
    connect(this->compressionWatcher, SIGNAL(finished()), progressDialog, SLOT(close()));
    connect(this->compressionWatcher, SIGNAL(finished()), progressDialog, SLOT(deleteLater()));
    connect(this->compressionWatcher, SIGNAL(finished()), this, SLOT(compressionFinished()));
    connect(this->compressionWatcher, SIGNAL(progressValueChanged(int)), progressDialog, SLOT(setValue(int)));
    connect(this->compressionWatcher, SIGNAL(progressValueChanged(int)), this->cImageModel, SLOT(emitDataChanged(int)));
    progressDialog->show();

    FileDatesOutputOption datesMap = {
        ui->keepCreationDate_CheckBox->isChecked(),
        ui->keepLastModifiedDate_CheckBox->isChecked(),
        ui->keepLastAccessDate_CheckBox->isChecked()
    };

    CompressionOptions compressionOptions = {
        this->ui->outputFolder_LineEdit->text(),
        rootFolder,
        this->ui->outputSuffix_LineEdit->text(),
        this->ui->lossless_CheckBox->isChecked(),
        this->ui->keepMetadata_CheckBox->isChecked(),
        this->ui->keepStructure_CheckBox->isChecked(),
        this->ui->fitTo_ComboBox->currentIndex() != ResizeMode::NO_RESIZE,
        this->ui->fitTo_ComboBox->currentIndex(),
        this->ui->width_SpinBox->value(),
        this->ui->height_SpinBox->value(),
        this->ui->edge_SpinBox->value(),
        this->ui->doNotEnlarge_CheckBox->isChecked(),
        this->ui->sameOutputFolderAsInput_CheckBox->isChecked(),
        qBound(this->ui->JPEGQuality_Slider->value(), 1, 100),
        qBound(this->ui->PNGLevel_Slider->value(), 1, 7),
        qBound(this->ui->WebPQuality_Slider->value(), 1, 100),
        ui->keepDates_CheckBox->checkState() != Qt::Unchecked,
        datesMap
    };

    this->compressionWatcher->setFuture(this->cImageModel->getRootItem()->compress(compressionOptions));
    compressionSummary.totalImages = this->cImageModel->rowCount();
    compressionSummary.totalUncompressedSize = this->cImageModel->originalItemsSize();
    compressionSummary.totalCompressedSize = 0;
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
    QSettings settings;
    if (settings.value("preferences/general/prompt_before_exit", false).toBool()) {
        QCaesiumMessageBox exitPrompt;
        exitPrompt.setText(tr("Do you really want to quit?"));
        exitPrompt.setInformativeText("All current work will be lost.");
        exitPrompt.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        exitPrompt.setButtonText(QMessageBox::Yes, tr("Yes"));
        exitPrompt.setButtonText(QMessageBox::No, tr("Cancel"));

        int answer = exitPrompt.exec();

        if (answer == QMessageBox::No) {
            event->ignore();
            return;
        }
    }

    this->writeSettings();
    qInfo() << "---- Closing application ----";
    event->accept();
}

void MainWindow::on_outputFolderBrowse_Button_clicked()
{
    QString directoryPath = QFileDialog::getExistingDirectory(this, tr("Select output folder..."),
        this->ui->outputFolder_LineEdit->text(),
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

void MainWindow::imageList_selectionChanged()
{
    auto currentSelectedIndexes = ui->imageList_TreeView->selectionModel()->selectedIndexes();

    if (currentSelectedIndexes.count() / 4 == 0) {
        this->previewScene->clear();
        this->compressedPreviewScene->clear();
        return;
    }

    auto currentIndex = currentSelectedIndexes.at(0);

    if (currentIndex.row() == -1) {
        return;
    }

    this->previewImage(currentIndex);
}

void MainWindow::compressionFinished()
{
    if (ui->imageList_TreeView->selectionModel()->selectedRows().count() > 0) {
        this->previewImage(ui->imageList_TreeView->selectionModel()->selectedRows().at(0));
    }

    compressionSummary.totalCompressedSize = this->cImageModel->compressedItemsSize();
    QCaesiumMessageBox compressionSummaryDialog;
    compressionSummaryDialog.setText(tr("Compression finished!"));
    compressionSummaryDialog.setInformativeText(tr("Total files: %1\nOriginal size: %2\nCompressed size: %3\nSaved: %4 (%5%)")
                                                    .arg(QString::number(compressionSummary.totalImages),
                                                        toHumanSize(compressionSummary.totalUncompressedSize),
                                                        toHumanSize(compressionSummary.totalCompressedSize),
                                                        toHumanSize(compressionSummary.totalUncompressedSize - compressionSummary.totalCompressedSize),
                                                        QString::number(round(((double)compressionSummary.totalUncompressedSize - (double)compressionSummary.totalCompressedSize) / (double)compressionSummary.totalUncompressedSize * 100))));
    compressionSummaryDialog.setStandardButtons(QMessageBox::Ok);
    compressionSummaryDialog.exec();
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
    case ResizeMode::NO_RESIZE:
        ui->resize_Frame->setDisabled(true);
        break;
    case ResizeMode::DIMENSIONS:
        ui->resize_Frame->setDisabled(false);
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
        ui->resize_Frame->setDisabled(false);
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
        ui->resize_Frame->setDisabled(false);
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
    this->writeSetting("compression_options/output/same_folder_as_input", checked);
}

void MainWindow::on_keepStructure_CheckBox_toggled(bool checked)
{
    this->writeSetting("compression_options/compression/keep_structure", checked);
}

void MainWindow::on_lossless_CheckBox_toggled(bool checked)
{
    this->writeSetting("compression_options/compression/lossless", checked);
}

void MainWindow::on_keepMetadata_CheckBox_toggled(bool checked)
{
    this->writeSetting("compression_options/compression/keep_metadata", checked);
}

void MainWindow::on_JPEGQuality_Slider_valueChanged(int value)
{
    this->writeSetting("compression_options/compression/jpeg_quality", value);
}

void MainWindow::on_JPEGQuality_SpinBox_valueChanged(int value)
{
    this->writeSetting("compression_options/compression/jpeg_quality", value);
}

void MainWindow::on_PNGLevel_Slider_valueChanged(int value)
{
    this->writeSetting("compression_options/compression/png_level", value);
}

void MainWindow::on_PNGLevel_SpinBox_valueChanged(int value)
{
    this->writeSetting("compression_options/compression/png_level", value);
}

void MainWindow::cModelItemsChanged()
{
    int itemsCount = this->cImageModel->rowCount();
    size_t totalOriginalSizeCount = this->cImageModel->originalItemsSize();
    QString humanItemsCount = QString::number(itemsCount);
    QString totalSize = toHumanSize(totalOriginalSizeCount);
    ui->statusbar->showMessage(humanItemsCount + " " + tr("images in list") + " | " + totalSize);

    ui->removeFiles_Button->setDisabled(itemsCount == 0);
    ui->actionRemove->setDisabled(itemsCount == 0);
    ui->actionClear->setDisabled(itemsCount == 0);
    ui->actionSelect_All->setDisabled(itemsCount == 0);
    ui->compress_Button->setDisabled(itemsCount == 0);
}

void MainWindow::initUpdater()
{
    QSettings settings;
#ifdef Q_OS_MAC
    CocoaInitializer initializer;
    auto updater = new SparkleAutoUpdater("https://saerasoft.com/repository/com.saerasoft.caesium/osx/appcast.xml");
    updater->setCheckForUpdatesAutomatically(settings.value("preferences/general/check_updates_at_startup", false).toBool());
    if (settings.value("preferences/general/check_updates_at_startup", false).toBool()) {
        updater->checkForUpdates();
    }
#endif

#ifdef Q_OS_WIN
    win_sparkle_set_appcast_url("https://saerasoft.com/repository/com.saerasoft.caesium/win/appcast.xml");
    win_sparkle_set_langid(QLocale().name().toShort());
    win_sparkle_init();

    if (settings.value("preferences/general/check_updates_at_startup", false).toBool()) {
        win_sparkle_check_update_without_ui();
    }

#endif
}

void MainWindow::on_actionShow_previews_toggled(bool toggled)
{
    this->writeSetting("mainwindow/previews_visible", toggled);

    // TODO If manually collapsed, this is inconsistent
    if (!toggled) {
        this->writeSetting("mainwindow/main_splitter_sizes", QVariant::fromValue<QList<int>>(ui->main_VSplitter->sizes()));
        ui->main_VSplitter->setSizes(QList<int>({ 500, 0 }));
    } else {
        QSettings settings;
        ui->main_VSplitter->setSizes(settings.value("mainwindow/main_splitter_sizes", QVariant::fromValue<QList<int>>({ 500, 250 })).value<QList<int>>());
        if (ui->imageList_TreeView->selectionModel()->selectedRows().count() > 0) {
            this->previewImage(ui->imageList_TreeView->selectionModel()->selectedRows().at(0));
        }
    }
}

void MainWindow::showListContextMenu(const QPoint& pos)
{
    this->listContextMenu->exec(ui->imageList_TreeView->viewport()->mapToGlobal(pos));
}

void MainWindow::on_actionPreferences_triggered()
{
    auto* preferencesDialog = new PreferencesDialog(this);
    preferencesDialog->setModal(true);

    preferencesDialog->show();
}

void MainWindow::keepDatesButtonGroupClicked()
{
    int checkedCount = 0;
    int totalButtons = (int)this->keepDatesButtonGroup->buttons().count();
    Qt::CheckState mainCheckboxState = Qt::PartiallyChecked;
    foreach (QAbstractButton* button, this->keepDatesButtonGroup->buttons()) {
        checkedCount += button->isChecked() ? 1 : 0;
    }

    if (checkedCount == totalButtons) {
        mainCheckboxState = Qt::Checked;
    } else if (checkedCount == 0) {
        mainCheckboxState = Qt::Unchecked;
    }

    ui->keepDates_CheckBox->setCheckState(mainCheckboxState);

    this->writeSetting("compression_options/output/keep_creation_date", ui->keepCreationDate_CheckBox->isChecked());
    this->writeSetting("compression_options/output/keep_last_modified_date", ui->keepLastModifiedDate_CheckBox->isChecked());
    this->writeSetting("compression_options/output/keep_last_access_date", ui->keepLastAccessDate_CheckBox->isChecked());
}

void MainWindow::on_keepDates_CheckBox_clicked()
{
    if (ui->keepDates_CheckBox->checkState() == Qt::PartiallyChecked) {
        return;
    }

    bool mainChecked = ui->keepDates_CheckBox->checkState() == Qt::Checked;
    foreach (QAbstractButton* button, this->keepDatesButtonGroup->buttons()) {
        button->setChecked(mainChecked);
    }

    this->writeSetting("compression_options/output/keep_creation_date", ui->keepCreationDate_CheckBox->isChecked());
    this->writeSetting("compression_options/output/keep_last_modified_date", ui->keepLastModifiedDate_CheckBox->isChecked());
    this->writeSetting("compression_options/output/keep_last_access_date", ui->keepLastAccessDate_CheckBox->isChecked());
}

void MainWindow::on_keepDates_CheckBox_stateChanged(int state)
{
    this->writeSetting("compression_options/output/keep_dates", state);
}
