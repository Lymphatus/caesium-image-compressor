#include "MainWindow.h"
#include "./delegates/HtmlDelegate.h"
#include "./exceptions/ImageNotSupportedException.h"
#include "ui_MainWindow.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QObject>
#include <QProgressDialog>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTime>
#include <QWheelEvent>
#include <QWidgetAction>
#include <QWindow>
#include <QtConcurrent>
#include <dialogs/PreferencesDialog.h>
#include <utility>
#include <widgets/QCaesiumMessageBox.h>
#include <QProgressBar>

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
    qInfo() << "Starting UI";

    this->cImageModel = new CImageTreeModel();
    this->previewScene = new QGraphicsScene();
    this->compressedPreviewScene = new QGraphicsScene();
    this->aboutDialog = new AboutDialog(this);
    this->compressionWatcher = new QFutureWatcher<void>();
    this->previewWatcher = new QFutureWatcher<QPixmap>();
    this->listContextMenu = new QMenu();
    this->networkOperations = new NetworkOperations();
    this->proxyModel = new CImageSortFilterProxyModel();

#ifdef Q_OS_MAC
    QIcon icon = QIcon(":/icons/logo_mono.png");
    icon.setIsMask(true);
    this->trayIcon = new QSystemTrayIcon(icon);
#endif

#ifdef Q_OS_WIN
    QIcon icon = QIcon(":/icons/logo.png");
    this->trayIcon = new QSystemTrayIcon(icon);
#endif

    ui->preview_GraphicsView->setScene(this->previewScene);
    ui->previewCompressed_GraphicsView->setScene(this->compressedPreviewScene);

    this->initListWidget();

    ui->edge_Label->hide();
    ui->edge_SpinBox->hide();

    this->keepDatesButtonGroup = new QButtonGroup();
    this->keepDatesButtonGroup->setExclusive(false);
    this->keepDatesButtonGroup->addButton(ui->keepCreationDate_CheckBox);
    this->keepDatesButtonGroup->addButton(ui->keepLastModifiedDate_CheckBox);
    this->keepDatesButtonGroup->addButton(ui->keepLastAccessDate_CheckBox);

    this->initStatusBar();
    this->initListContextMenu();

    connect(ui->imageList_TreeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::imageList_selectionChanged);
    connect(ui->imageList_TreeView, &QDropTreeView::dropFinished, this, &MainWindow::dropFinished);
    connect(this->cImageModel, &CImageTreeModel::itemsChanged, this, &MainWindow::cModelItemsChanged);
    connect(ui->preview_GraphicsView, &QZoomGraphicsView::scaleFactorChanged, ui->previewCompressed_GraphicsView, &QZoomGraphicsView::setScaleFactor);
    connect(ui->previewCompressed_GraphicsView, &QZoomGraphicsView::scaleFactorChanged, ui->preview_GraphicsView, &QZoomGraphicsView::setScaleFactor);
    connect(ui->imageList_TreeView, &QWidget::customContextMenuRequested, this, &MainWindow::showListContextMenu);
    connect(ui->imageList_TreeView->header(), &QHeaderView::sortIndicatorChanged, this, &MainWindow::listSortChanged);

    connect(ui->preview_GraphicsView->horizontalScrollBar(), &QAbstractSlider::valueChanged, ui->previewCompressed_GraphicsView, &QZoomGraphicsView::setHorizontalScrollBarValue);
    connect(ui->preview_GraphicsView->verticalScrollBar(), &QAbstractSlider::valueChanged, ui->previewCompressed_GraphicsView, &QZoomGraphicsView::setVerticalScrollBarValue);
    connect(ui->previewCompressed_GraphicsView->horizontalScrollBar(), &QAbstractSlider::valueChanged, ui->preview_GraphicsView, &QZoomGraphicsView::setHorizontalScrollBarValue);
    connect(ui->previewCompressed_GraphicsView->verticalScrollBar(), &QAbstractSlider::valueChanged, ui->preview_GraphicsView, &QZoomGraphicsView::setVerticalScrollBarValue);
    connect(keepDatesButtonGroup, &QButtonGroup::buttonClicked, this, &MainWindow::keepDatesButtonGroupClicked);

    connect(this->previewWatcher, &QFutureWatcher<QPixmap>::resultReadyAt, this, &MainWindow::showPreview);
    this->readSettings();

    this->on_fitTo_ComboBox_currentIndexChanged(ui->fitTo_ComboBox->currentIndex());
    this->on_keepAspectRatio_CheckBox_toggled(ui->keepAspectRatio_CheckBox->isChecked());
    this->on_doNotEnlarge_CheckBox_toggled(ui->doNotEnlarge_CheckBox->isChecked());
    this->on_keepAspectRatio_CheckBox_toggled(ui->keepAspectRatio_CheckBox->isChecked());
    this->on_sameOutputFolderAsInput_CheckBox_toggled(ui->sameOutputFolderAsInput_CheckBox->isChecked());

    ui->actionToolbarIcons_only->setChecked(ui->toolBar->toolButtonStyle() == Qt::ToolButtonIconOnly && !ui->toolBar->isHidden());
    ui->actionToolbarIcons_and_Text->setChecked(ui->toolBar->toolButtonStyle() == Qt::ToolButtonTextUnderIcon && !ui->toolBar->isHidden());
    ui->actionToolbarHide->setChecked(ui->toolBar->isHidden());

    QSettings settings;
    if (settings.value("preferences/general/send_usage_reports", false).toBool()) {
        if (!settings.contains("access_token")) {
            this->networkOperations->requestToken();
        } else {
            this->networkOperations->updateSystemInfo();
        }
    }

    // TODO Move to a function
    this->trayIcon->setContextMenu(new QMenu());
    this->trayIcon->show();

    QImageReader::setAllocationLimit(512);
}

MainWindow::~MainWindow()
{
#ifdef Q_OS_WIN
    win_sparkle_cleanup();
#endif

    delete proxyModel;
    delete cImageModel;
    delete previewScene;
    delete compressedPreviewScene;
    delete aboutDialog;
    delete keepDatesButtonGroup;
    delete compressionWatcher;
    delete networkOperations;
    delete previewWatcher;
    delete trayIcon;
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
    this->listContextMenu->addSeparator();
    this->listContextMenu->addAction(ui->actionShow_original_in_file_manager);
    this->listContextMenu->addAction(ui->actionShow_compressed_in_file_manager);

    connect(this->listContextMenu, &QMenu::aboutToShow, this, &MainWindow::listContextMenuAboutToShow);
}

void MainWindow::initListWidget()
{
    QSettings settings;
    int defaultSectionSize = ui->imageList_TreeView->header()->defaultSectionSize();
    this->proxyModel->setSourceModel(this->cImageModel);
    ui->imageList_TreeView->setModel(this->proxyModel);
    ui->imageList_TreeView->setIconSize(QSize(10, 10));
    ui->imageList_TreeView->header()->resizeSection(CImageColumns::NAME_COLUMN, settings.value("mainwindow/list_view/header_column_size/name", 250).toInt());
    ui->imageList_TreeView->header()->resizeSection(CImageColumns::SIZE_COLUMN, settings.value("mainwindow/list_view/header_column_size/size", defaultSectionSize).toInt());
    ui->imageList_TreeView->header()->resizeSection(CImageColumns::RESOLUTION_COLUMN, settings.value("mainwindow/list_view/header_column_size/resolution", defaultSectionSize).toInt());
    ui->imageList_TreeView->header()->resizeSection(CImageColumns::RATIO_COLUMN, settings.value("mainwindow/list_view/header_column_size/ratio", defaultSectionSize).toInt());

    ui->imageList_TreeView->header()->setSortIndicator(settings.value("mainwindow/list_view/sort_column_index", 0).toInt(), settings.value("mainwindow/list_view/sort_column_order", Qt::AscendingOrder).value<Qt::SortOrder>());

    ui->imageList_TreeView->header()->resizeSection(CImageColumns::RATIO_COLUMN, settings.value("mainwindow/list_view/header_column_size/ratio", defaultSectionSize).toInt());
    ui->imageList_TreeView->header()->resizeSection(CImageColumns::RATIO_COLUMN, settings.value("mainwindow/list_view/header_column_size/ratio", defaultSectionSize).toInt());
    ui->imageList_TreeView->setItemDelegate(new HtmlDelegate());
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

    QSettings settings;
    bool scanSubfolders = settings.value("preferences/general/import_subfolders", true).toBool();
    QStringList fileList = scanDirectory(directoryPath, scanSubfolders);

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
    settings.setValue("mainwindow/left_splitter_sizes", QVariant::fromValue<QList<int>>(ui->sidebar_HSplitter->sizes()));
    settings.setValue("mainwindow/previews_visible", ui->actionShow_previews->isChecked());
    if (ui->actionShow_previews->isChecked()) {
        settings.setValue("mainwindow/main_splitter_sizes", QVariant::fromValue<QList<int>>(ui->main_VSplitter->sizes()));
    }
    settings.setValue("mainwindow/list_view/header_column_size/name", ui->imageList_TreeView->header()->sectionSize(CImageColumns::NAME_COLUMN));
    settings.setValue("mainwindow/list_view/header_column_size/size", ui->imageList_TreeView->header()->sectionSize(CImageColumns::SIZE_COLUMN));
    settings.setValue("mainwindow/list_view/header_column_size/resolution", ui->imageList_TreeView->header()->sectionSize(CImageColumns::RESOLUTION_COLUMN));
    settings.setValue("mainwindow/list_view/header_column_size/ratio", ui->imageList_TreeView->header()->sectionSize(CImageColumns::RATIO_COLUMN));
    settings.setValue("mainwindow/list_view/sort_column_index", ui->imageList_TreeView->header()->sortIndicatorSection());
    settings.setValue("mainwindow/list_view/sort_column_order", ui->imageList_TreeView->header()->sortIndicatorOrder());
    settings.setValue("mainwindow/toolbar/visible", ui->toolBar->isVisible());
    settings.setValue("mainwindow/toolbar/button_style", ui->toolBar->toolButtonStyle());

    settings.setValue("compression_options/compression/lossless", ui->lossless_CheckBox->isChecked());
    settings.setValue("compression_options/compression/keep_metadata", ui->keepMetadata_CheckBox->isChecked());
    settings.setValue("compression_options/compression/keep_structure", ui->keepStructure_CheckBox->isChecked());
    settings.setValue("compression_options/compression/jpeg_quality", ui->JPEGQuality_Slider->value());
    settings.setValue("compression_options/compression/png_level", ui->PNGLevel_Slider->value());
    settings.setValue("compression_options/compression/webp_quality", ui->WebPQuality_Slider->value());

    settings.setValue("compression_options/resize/resize", ui->fitTo_ComboBox->currentIndex() != ResizeMode::NO_RESIZE);
    settings.setValue("compression_options/resize/fit_to", ui->fitTo_ComboBox->currentIndex());
    settings.setValue("compression_options/resize/width", ui->width_SpinBox->value());
    settings.setValue("compression_options/resize/height", ui->height_SpinBox->value());
    settings.setValue("compression_options/resize/size", ui->edge_SpinBox->value());
    settings.setValue("compression_options/resize/keep_aspect_ratio", ui->keepAspectRatio_CheckBox->isChecked());
    settings.setValue("compression_options/resize/do_not_enlarge", ui->doNotEnlarge_CheckBox->isChecked());

    settings.setValue("compression_options/output/output_folder", ui->outputFolder_LineEdit->text());
    settings.setValue("compression_options/output/output_suffix", ui->outputSuffix_LineEdit->text());
    settings.setValue("compression_options/output/same_folder_as_input", ui->sameOutputFolderAsInput_CheckBox->isChecked());
    settings.setValue("compression_options/output/keep_dates", ui->keepDates_CheckBox->checkState());
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

    ui->sidebar_HSplitter->setSizes(settings.value("mainwindow/left_splitter_sizes", QVariant::fromValue<QList<int>>({ 600, 1 })).value<QList<int>>());
    ui->main_VSplitter->setSizes(settings.value("mainwindow/main_splitter_sizes", QVariant::fromValue<QList<int>>({ 500, 250 })).value<QList<int>>());
    ui->actionShow_previews->setChecked(settings.value("mainwindow/previews_visible", true).toBool());
    ui->toolBar->setVisible(settings.value("mainwindow/toolbar/visible", true).toBool());
    ui->toolBar->setToolButtonStyle(settings.value("mainwindow/toolbar/button_style", Qt::ToolButtonIconOnly).value<Qt::ToolButtonStyle>());

    ui->lossless_CheckBox->setChecked(settings.value("compression_options/compression/lossless", false).toBool());
    ui->keepMetadata_CheckBox->setChecked(settings.value("compression_options/compression/keep_metadata", true).toBool());
    ui->keepStructure_CheckBox->setChecked(settings.value("compression_options/compression/keep_structure", false).toBool());
    ui->JPEGQuality_Slider->setValue(settings.value("compression_options/compression/jpeg_quality", 80).toInt());
    ui->PNGLevel_SpinBox->setValue(settings.value("compression_options/compression/png_level", 3).toInt());
    ui->JPEGQuality_SpinBox->setValue(settings.value("compression_options/compression/jpeg_quality", 80).toInt());
    ui->WebPQuality_SpinBox->setValue(settings.value("compression_options/compression/webp_quality", 60).toInt());

    ui->fitTo_ComboBox->setCurrentIndex(settings.value("compression_options/resize/fit_to", 0).toInt());
    ui->width_SpinBox->setValue(settings.value("compression_options/resize/width", 1000).toInt());
    ui->height_SpinBox->setValue(settings.value("compression_options/resize/height", 1000).toInt());
    ui->edge_SpinBox->setValue(settings.value("compression_options/resize/size", 1000).toInt());
    ui->keepAspectRatio_CheckBox->setChecked(settings.value("compression_options/resize/keep_aspect_ratio", false).toBool());
    ui->doNotEnlarge_CheckBox->setChecked(settings.value("compression_options/resize/do_not_enlarge", false).toBool());

    ui->outputFolder_LineEdit->setText(settings.value("compression_options/output/output_folder", "").toString());
    ui->outputSuffix_LineEdit->setText(settings.value("compression_options/output/output_suffix", "").toString());
    ui->sameOutputFolderAsInput_CheckBox->setChecked(settings.value("compression_options/output/same_folder_as_input", false).toBool());
    ui->keepDates_CheckBox->setCheckState(settings.value("compression_options/output/keep_dates", Qt::Unchecked).value<Qt::CheckState>());
    ui->keepCreationDate_CheckBox->setChecked(settings.value("compression_options/output/keep_creation_date", false).toBool());
    ui->keepLastModifiedDate_CheckBox->setChecked(settings.value("compression_options/output/keep_last_modified_date", false).toBool());
    ui->keepLastAccessDate_CheckBox->setChecked(settings.value("compression_options/output/keep_last_access_date", false).toBool());

    this->lastOpenedDirectory = settings.value("extra/last_opened_directory", QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0)).toString();
}

void MainWindow::previewImage(const QModelIndex& imageIndex)
{
    if (this->previewWatcher->isRunning()) {
        this->previewWatcher->cancel();
        this->previewWatcher->waitForFinished();
    }
    QSettings settings;
    if (!settings.value("mainwindow/previews_visible", false).toBool()) {
        return;
    }
    this->previewScene->clear();
    this->compressedPreviewScene->clear();

    ui->preview_GraphicsView->resetScaleFactor();
    ui->preview_GraphicsView->setLoading(true);
    this->previewScene->invalidate(QRect(), QGraphicsScene::ForegroundLayer);
    ui->previewCompressed_GraphicsView->resetScaleFactor();

    CImage* cImage = this->cImageModel->getRootItem()->children().at(imageIndex.row())->getCImage();
    QStringList images = QStringList() << cImage->getFullPath();

    std::function<QPixmap(const QString&)> loadPixmap = [](const QString& imageFileName) {
        QPixmap image(imageFileName);
        return image;
    };

    if (!cImage->getCompressedFullPath().isEmpty()) {
        images.append(cImage->getCompressedFullPath());
        ui->previewCompressed_GraphicsView->setLoading(true);
        this->compressedPreviewScene->invalidate(QRect(), QGraphicsScene::ForegroundLayer);
    } else {
        ui->previewCompressed_GraphicsView->setLoading(false);
        this->compressedPreviewScene->setSceneRect(this->previewScene->itemsBoundingRect());
        ui->previewCompressed_GraphicsView->fitInView(this->previewScene->itemsBoundingRect(), Qt::KeepAspectRatio);
        ui->previewCompressed_GraphicsView->show();
    }

    this->previewWatcher->setFuture(QtConcurrent::mapped(images, loadPixmap));
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

void MainWindow::importFiles(const QStringList& fileList, QString baseFolder)
{
    int listLength = (int)fileList.count();
    QProgressDialog progressDialog(tr("Importing files..."), tr("Cancel"), 0, listLength, this);
    progressDialog.setWindowModality(Qt::WindowModal);

    QList<CImage*> list;
    for (int i = 0; i < listLength; i++) {
        if (progressDialog.wasCanceled()) {
            break;
        }

        try {
            auto* cImage = new CImage(fileList.at(i));
            if (this->cImageModel->contains(cImage)) {
                continue;
            }
            list.append(cImage);
        } catch (ImageNotSupportedException& e) {
            qWarning() << fileList.at(i) << "is not supported";
        }

        progressDialog.setValue(i);
    }

    if (!list.isEmpty() && listLength > 0) {
        this->updateFolderMap(std::move(baseFolder), (int)list.count());
        QString rootFolder = getRootFolder(this->folderMap);
        this->cImageModel->appendItems(list, rootFolder);
        this->importedFilesRootFolder = getRootFolder(this->folderMap);
    }

    progressDialog.setValue(listLength);
}

void MainWindow::removeFiles(bool all)
{
    this->isItemRemovalRunning = true;
    if (all) {
        ui->imageList_TreeView->selectAll();
    }
    QModelIndexList indexes = ui->imageList_TreeView->selectionModel()->selectedIndexes();
    std::sort(indexes.begin(), indexes.end(), [](const QModelIndex& a, const QModelIndex& b) {
        return a.row() < b.row();
    });

    int columnCount = this->cImageModel->columnCount();

    for (int i = indexes.count() / columnCount; i > 0; i--) {
        auto currentIndex = this->proxyModel->mapToSource(indexes.at(i));
        auto indexRow = currentIndex.row();
        auto indexParent = currentIndex.parent();
        this->updateFolderMap(this->cImageModel->getRootItem()->children().at(indexRow)->getCImage()->getFullPath(), -1);
        this->cImageModel->removeRows(indexRow, 1, indexParent);
    }
    this->previewScene->clear();
    this->compressedPreviewScene->clear();
    this->previewScene->setSceneRect(this->previewScene->itemsBoundingRect());
    this->previewScene->setSceneRect(this->compressedPreviewScene->itemsBoundingRect());
    ui->preview_GraphicsView->fitInView(this->previewScene->itemsBoundingRect(), Qt::KeepAspectRatio);
    this->isItemRemovalRunning = false;
}

void MainWindow::on_compress_Button_clicked()
{
    this->startCompression();
}

void MainWindow::startCompression()
{
    QSettings settings;

    if (ui->outputFolder_LineEdit->text().isEmpty() && !ui->sameOutputFolderAsInput_CheckBox->isChecked()) {
        QCaesiumMessageBox msgBox;
        msgBox.setText("Please select an output folder first");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        return;
    }

    QString rootFolder = getRootFolder(this->folderMap);

    bool overwriteWarningFlag = (ui->sameOutputFolderAsInput_CheckBox->isChecked() && ui->outputSuffix_LineEdit->text().isEmpty())
        || rootFolder == ui->outputFolder_LineEdit->text();

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

    if (!settings.value("preferences/general/multithreading", true).toBool()) {
        QThreadPool::globalInstance()->setMaxThreadCount(1);
    } else {
        QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());
    }

    auto* progressDialog = new QProgressDialog(tr("Compressing..."), tr("Cancel"), 0, this->cImageModel->getRootItem()->childCount(), this);
    progressDialog->setWindowModality(Qt::WindowModal);

    this->compressionWatcher = new QFutureWatcher<void>();
    connect(this->compressionWatcher, &QFutureWatcherBase::finished, progressDialog, &QObject::deleteLater);
    connect(this->compressionWatcher, &QFutureWatcherBase::finished, this, &MainWindow::compressionFinished);
    connect(this->compressionWatcher, &QFutureWatcherBase::progressValueChanged, progressDialog, &QProgressDialog::setValue);
    connect(this->compressionWatcher, &QFutureWatcherBase::progressValueChanged, this->cImageModel, &CImageTreeModel::emitDataChanged);

    connect(progressDialog, &QProgressDialog::canceled, this, &MainWindow::compressionCanceled);
    progressDialog->show();

    FileDatesOutputOption datesMap = {
        ui->keepCreationDate_CheckBox->isChecked(),
        ui->keepLastModifiedDate_CheckBox->isChecked(),
        ui->keepLastAccessDate_CheckBox->isChecked()
    };

    CompressionOptions compressionOptions = {
        ui->outputFolder_LineEdit->text(),
        rootFolder,
        ui->outputSuffix_LineEdit->text(),
        ui->lossless_CheckBox->isChecked(),
        ui->keepMetadata_CheckBox->isChecked(),
        ui->keepStructure_CheckBox->isChecked(),
        ui->fitTo_ComboBox->currentIndex() != ResizeMode::NO_RESIZE,
        ui->fitTo_ComboBox->currentIndex(),
        ui->width_SpinBox->value(),
        ui->height_SpinBox->value(),
        ui->edge_SpinBox->value(),
        ui->doNotEnlarge_CheckBox->isChecked(),
        ui->sameOutputFolderAsInput_CheckBox->isChecked(),
        qBound(ui->JPEGQuality_Slider->value(), 1, 100),
        qBound(ui->PNGLevel_Slider->value(), 1, 7),
        qBound(ui->WebPQuality_Slider->value(), 1, 100),
        ui->keepDates_CheckBox->checkState() != Qt::Unchecked,
        datesMap
    };

    this->compressionWatcher->setFuture(this->cImageModel->getRootItem()->compress(compressionOptions));
    compressionSummary.totalImages = this->cImageModel->rowCount();
    compressionSummary.totalUncompressedSize = this->cImageModel->originalItemsSize();
    compressionSummary.totalCompressedSize = 0;
    compressionSummary.elapsedTime = 0;

    compressionTimer.start();
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
        ui->outputFolder_LineEdit->text(),
        QFileDialog::ShowDirsOnly);

    if (!directoryPath.isEmpty()) {
        ui->outputFolder_LineEdit->setText(directoryPath);

        this->writeSetting("compression_options/output/output_folder", directoryPath);
    }
}

void MainWindow::on_outputSuffix_LineEdit_textChanged(const QString& arg1)
{
    this->writeSetting("compression_options/output/output_suffix", arg1);
}

void MainWindow::imageList_selectionChanged()
{
    this->selectedIndexes = ui->imageList_TreeView->selectionModel()->selectedIndexes();
    this->selectedCount = this->selectedIndexes.count() / CIMAGE_COLUMNS_SIZE;
    if (this->isItemRemovalRunning) {
        return;
    }
    ui->actionRemove->setDisabled(this->selectedCount == 0);
    ui->removeFiles_Button->setDisabled(this->selectedCount == 0);
    ui->actionShow_original_in_file_manager->setEnabled(this->selectedCount == 1);
    ui->actionShow_compressed_in_file_manager->setEnabled(this->selectedCount == 1);
    if (this->selectedCount == 0) {
        this->previewScene->clear();
        this->compressedPreviewScene->clear();
        return;
    }

    auto currentIndex = this->selectedIndexes.at(0);

    if (currentIndex.row() == -1) {
        return;
    }

    this->previewImage(this->proxyModel->mapToSource(currentIndex));
}

void MainWindow::compressionFinished()
{
    this->cImageModel->getRootItem()->setCompressionCanceled(false);
    QSettings settings;
    if (ui->imageList_TreeView->selectionModel()->selectedRows().count() > 0) {
        this->previewImage(this->proxyModel->mapToSource(ui->imageList_TreeView->selectionModel()->selectedRows().at(0)));
    }

    compressionSummary.totalCompressedSize = this->cImageModel->compressedItemsSize();
    compressionSummary.elapsedTime = compressionTimer.isValid() ? compressionTimer.elapsed() : 0;

    if (settings.value("preferences/general/send_usage_reports", true).toBool()) {
        this->networkOperations->sendUsageReport(compressionSummary);
    }

    qInfo() << "Compression ended successfully.\nTotal images:" << compressionSummary.totalImages
            << "\nUncompressed size:" << toHumanSize(compressionSummary.totalUncompressedSize)
            << "\nCompressed size:" << toHumanSize(compressionSummary.totalCompressedSize)
            << "\nElapsed time:" << compressionSummary.elapsedTime << "ms";

    QString title = tr("Compression finished!");
    QString saved = toHumanSize(compressionSummary.totalUncompressedSize - compressionSummary.totalCompressedSize);
    QString savedPerc = QString::number(round((compressionSummary.totalUncompressedSize - compressionSummary.totalCompressedSize) / compressionSummary.totalUncompressedSize * 100));
    this->trayIcon->showMessage(title, tr("You just saved: %1!").arg(saved), QSystemTrayIcon::NoIcon);

    QCaesiumMessageBox compressionSummaryDialog;
    compressionSummaryDialog.setText(title);
    compressionSummaryDialog.setInformativeText(tr("Total files: %1\nOriginal size: %2\nCompressed size: %3\nSaved: %4 (%5%)")
                                                    .arg(QString::number(compressionSummary.totalImages),
                                                        toHumanSize(compressionSummary.totalUncompressedSize),
                                                        toHumanSize(compressionSummary.totalCompressedSize),
                                                        saved,
                                                        savedPerc));
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
    if (ui->fitTo_ComboBox->currentIndex() == ResizeMode::PERCENTAGE && ui->keepAspectRatio_CheckBox->isChecked()) {
        ui->height_SpinBox->setValue(value);
    }
    this->writeSetting("compression_options/resize/width", value);
    this->writeSetting("compression_options/resize/height", ui->height_SpinBox->value());
}

void MainWindow::on_height_SpinBox_valueChanged(int value)
{
    if (ui->fitTo_ComboBox->currentIndex() == ResizeMode::PERCENTAGE && ui->keepAspectRatio_CheckBox->isChecked()) {
        ui->width_SpinBox->setValue(value);
    }
    this->writeSetting("compression_options/resize/height", value);
    this->writeSetting("compression_options/resize/width", ui->width_SpinBox->value());
}

void MainWindow::on_edge_SpinBox_valueChanged(int value)
{
    this->writeSetting("compression_options/resize/size", value);
}

void MainWindow::on_keepAspectRatio_CheckBox_toggled(bool checked)
{
    if (ui->fitTo_ComboBox->currentIndex() == ResizeMode::PERCENTAGE && checked) {
        ui->height_SpinBox->setValue(ui->width_SpinBox->value());
    }
    this->writeSetting("compression_options/resize/keep_aspect_ratio", checked);
}

void MainWindow::on_doNotEnlarge_CheckBox_toggled(bool checked)
{
    if (ui->fitTo_ComboBox->currentIndex() == ResizeMode::PERCENTAGE && checked) {
        ui->width_SpinBox->setMaximum(100);
        ui->height_SpinBox->setMaximum(100);
    } else {
        int maximum = ui->fitTo_ComboBox->currentIndex() == ResizeMode::PERCENTAGE ? 999 : 99999;
        ui->width_SpinBox->setMaximum(maximum);
        ui->height_SpinBox->setMaximum(maximum);
    }
    this->writeSetting("compression_options/resize/do_not_enlarge", checked);
}

void MainWindow::on_actionSelect_All_triggered()
{
    ui->imageList_TreeView->selectAll();
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
    QString humanItemsCount = QString::number(itemsCount);
    QString totalSize = toHumanSize(this->cImageModel->originalItemsSize());
    ui->statusbar->showMessage(humanItemsCount + " " + tr("images in list") + " | " + totalSize);

    ui->removeFiles_Button->setEnabled(itemsCount > 0 && this->selectedCount > 0);
    ui->actionRemove->setEnabled(itemsCount > 0 && this->selectedCount > 0);
    ui->actionClear->setDisabled(itemsCount == 0);
    ui->actionSelect_All->setDisabled(itemsCount == 0);
    ui->compress_Button->setDisabled(itemsCount == 0);
    ui->actionCompress->setDisabled(itemsCount == 0);

    ui->actionShow_original_in_file_manager->setDisabled(itemsCount == 0);
    ui->actionShow_compressed_in_file_manager->setDisabled(itemsCount == 0);
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
    int localeIndex = settings.value("preferences/language/locale", 0).toInt();
    if (localeIndex < 0 || localeIndex > LANGUAGES_COUNT - 1) {
        localeIndex = 0;
    }
    if (localeIndex != 0) {
        win_sparkle_set_langid(WIN32_LANGUAGES[localeIndex]);
    }
    win_sparkle_set_appcast_url("https://saerasoft.com/repository/com.saerasoft.caesium/win/appcast.xml");
    win_sparkle_init();

    if (settings.value("preferences/general/check_updates_at_startup", false).toBool()) {
        win_sparkle_check_update_without_ui();
    }

#endif
}

void MainWindow::on_actionShow_previews_toggled(bool toggled)
{
    ui->main_VSplitter->setChildrenCollapsible(!toggled);
    ui->main_VSplitter->handle(1)->setEnabled(toggled);
    this->writeSetting("mainwindow/previews_visible", toggled);

    if (!toggled) {
        this->writeSetting("mainwindow/main_splitter_sizes", QVariant::fromValue<QList<int>>(ui->main_VSplitter->sizes()));
        ui->main_VSplitter->setSizes(QList<int>({ 500, 0 }));
    } else {
        QSettings settings;
        ui->main_VSplitter->setSizes(settings.value("mainwindow/main_splitter_sizes", QVariant::fromValue<QList<int>>({ 500, 250 })).value<QList<int>>());
        if (ui->imageList_TreeView->selectionModel()->selectedRows().count() > 0) {
            this->previewImage(this->proxyModel->mapToSource(ui->imageList_TreeView->selectionModel()->selectedRows().at(0)));
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

void MainWindow::on_actionShow_original_in_file_manager_triggered()
{
    if (this->selectedCount != 1) {
        return;
    }

    auto currentIndex = this->proxyModel->mapToSource(this->selectedIndexes.at(0));
    auto cImage = this->cImageModel->getRootItem()->children().at(currentIndex.row())->getCImage();
    showFileInNativeFileManager(cImage->getFullPath(), cImage->getDirectory());
}

void MainWindow::on_actionShow_compressed_in_file_manager_triggered()
{
    if (this->selectedCount != 1) {
        return;
    }

    auto currentIndex = this->proxyModel->mapToSource(this->selectedIndexes.at(0));
    auto cImage = this->cImageModel->getRootItem()->children().at(currentIndex.row())->getCImage();
    if (cImage->getCompressedFullPath().isEmpty()) {
        return;
    }
    showFileInNativeFileManager(cImage->getCompressedFullPath(), cImage->getCompressedDirectory());
}

void MainWindow::listContextMenuAboutToShow()
{
    if (this->selectedCount < 1) {
        return;
    }

    auto currentIndex = this->proxyModel->mapToSource(this->selectedIndexes.at(0));
    auto cImage = this->cImageModel->getRootItem()->children().at(currentIndex.row())->getCImage();
    ui->actionShow_compressed_in_file_manager->setDisabled(cImage->getCompressedFullPath().isEmpty());
}

void MainWindow::showPreview(int index)
{
    if (index == 0) {
        ui->preview_GraphicsView->setLoading(false);
        this->previewScene->addPixmap(previewWatcher->resultAt(index));
        this->previewScene->setSceneRect(this->previewScene->itemsBoundingRect());
        ui->preview_GraphicsView->fitInView(this->previewScene->itemsBoundingRect(), Qt::KeepAspectRatio);
        ui->preview_GraphicsView->show();
    }

    if (index == 1) {
        ui->previewCompressed_GraphicsView->setLoading(false);
        this->compressedPreviewScene->addPixmap(previewWatcher->resultAt(index));
        this->compressedPreviewScene->setSceneRect(this->compressedPreviewScene->itemsBoundingRect());
        ui->previewCompressed_GraphicsView->fitInView(this->compressedPreviewScene->itemsBoundingRect(), Qt::KeepAspectRatio);
        ui->previewCompressed_GraphicsView->show();
    }
}
void MainWindow::compressionCanceled()
{
    qInfo() << "Compression canceled by user.";
    this->cImageModel->getRootItem()->setCompressionCanceled(true);
    this->compressionWatcher->cancel();
    this->compressionWatcher->waitForFinished();
}

void MainWindow::listSortChanged(int logicalIndex, Qt::SortOrder order)
{
    this->writeSetting("mainwindow/list_view/sort_column_index", logicalIndex);
    this->writeSetting("mainwindow/list_view/sort_column_order", order);
}

void MainWindow::on_actionCompress_triggered()
{
    this->startCompression();
}

void MainWindow::on_actionDonate_triggered()
{
    QDesktopServices::openUrl(QUrl("https://saerasoft.com/caesium/donate", QUrl::TolerantMode));
}

void MainWindow::on_actionToolbarIcons_only_triggered()
{
    ui->actionToolbarIcons_and_Text->setChecked(false);
    ui->actionToolbarHide->setChecked(false);
    ui->actionToolbarIcons_only->setChecked(true);
    ui->toolBar->setVisible(true);
    ui->toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    this->writeSetting("mainwindow/toolbar/visible", true);
    this->writeSetting("mainwindow/toolbar/button_style", Qt::ToolButtonIconOnly);
}

void MainWindow::on_actionToolbarIcons_and_Text_triggered()
{
    ui->actionToolbarIcons_only->setChecked(false);
    ui->actionToolbarHide->setChecked(false);
    ui->actionToolbarIcons_and_Text->setChecked(true);
    ui->toolBar->setVisible(true);
    ui->toolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    this->writeSetting("mainwindow/toolbar/visible", true);
    this->writeSetting("mainwindow/toolbar/button_style", Qt::ToolButtonTextUnderIcon);
}

void MainWindow::on_actionToolbarHide_triggered()
{
    ui->actionToolbarIcons_only->setChecked(false);
    ui->actionToolbarIcons_and_Text->setChecked(false);
    ui->actionToolbarHide->setChecked(true);
    ui->toolBar->setVisible(false);
    this->writeSetting("mainwindow/toolbar/visible", false);
}
