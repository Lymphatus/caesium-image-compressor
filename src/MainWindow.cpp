#include "MainWindow.h"
#include "delegates/HtmlDelegate.h"
#include "exceptions/ImageNotSupportedException.h"
#include "exceptions/ImageTooBigException.h"
#include "ui_MainWindow.h"
#include "utils/LanguageManager.h"
#include "utils/Logger.h"
#include "utils/PostCompressionActions.h"
#include <QDesktopServices>
#include <QFileDialog>
#include <QMovie>
#include <QProgressBar>
#include <QProgressDialog>
#include <QScrollBar>
#include <QWheelEvent>
#include <QWindow>
#include <QtConcurrent>
#include <dialogs/AdvancedImportDialog.h>
#include <dialogs/PreferencesDialog.h>
#include <services/Importer.h>
#include <utility>
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
    qRegisterMetaType<QList<int>>();
    qRegisterMetaType<QWheelEvent*>();
    qRegisterMetaType<Qt::CheckState>();
    qRegisterMetaType<Qt::SortOrder>();

    ui->setupUi(this);
    qInfo() << "Starting UI";

    this->translator = new QTranslator();
    LanguageManager::loadLocale(translator);

    this->cImageModel = new CImageTreeModel();
    this->aboutDialog = new AboutDialog(this);
    this->compressionWatcher = new QFutureWatcher<void>();
    this->previewWatcher = new QFutureWatcher<ImagePreview>();
    this->listContextMenu = new QMenu();
    this->trayIconContextMenu = new QMenu();
    this->networkOperations = new NetworkOperations();
    this->proxyModel = new CImageSortFilterProxyModel();
    this->trayIcon = new QSystemTrayIcon();

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
    this->initTrayIconContextMenu();
    this->initTrayIcon();

    ui->format_ComboBox->addItems(getOutputSupportedFormats());
    this->setupChromaSubsamplingComboBox();

    ui->JPEGToggle_ToolButton->setContent(ui->JPEGOptions_Frame);
    ui->PNGToggle_ToolButton->setContent(ui->PNGOptions_Frame);
    ui->WebPToggle_ToolButton->setContent(ui->WebPOptions_Frame);
    ui->TIFFToggle_ToolButton->setContent(ui->TIFFOptions_Frame);

    connect(ui->JPEGToggle_ToolButton, &QCollapseToolButton::contentVisibilityToggled, this, &MainWindow::onJPEGOptionsVisibilityChanged);
    connect(ui->PNGToggle_ToolButton, &QCollapseToolButton::contentVisibilityToggled, this, &MainWindow::onPNGOptionsVisibilityChanged);
    connect(ui->WebPToggle_ToolButton, &QCollapseToolButton::contentVisibilityToggled, this, &MainWindow::onWebPOptionsVisibilityChanged);
    connect(ui->TIFFToggle_ToolButton, &QCollapseToolButton::contentVisibilityToggled, this, &MainWindow::onTIFFOptionsVisibilityChanged);

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

    connect(this->previewWatcher, &QFutureWatcher<ImagePreview>::resultReadyAt, this, &MainWindow::showPreview);
    connect(this->previewWatcher, &QFutureWatcher<ImagePreview>::finished, this, &MainWindow::previewFinished);
    connect(this->previewWatcher, &QFutureWatcher<ImagePreview>::canceled, this, &MainWindow::previewCanceled);
    connect(ui->compressionMode_ComboBox, &QComboBox::currentIndexChanged, ui->compression_StackedWidget, &QStackedWidget::setCurrentIndex);
    connect(ui->compressionMode_ComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::onCompressionModeChanged);
    connect(ui->maxOutputSize_SpinBox, &QSpinBox::valueChanged, this, &MainWindow::onMaxOutputSizeChanged);
    connect(ui->maxOutputSizeUnit_ComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::onMaxOutputSizeUnitChanged);

    connect(ui->actionAdvanced_import, &QAction::triggered, this, &MainWindow::onAdvancedImportTriggered);

    connect(ui->moveOriginalFile_CheckBox, &QCheckBox::toggled, this, &MainWindow::moveOriginalFileToggled);
    connect(ui->moveOriginalFile_ComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::moveOriginalFileDestinationChanged);

    connect(ui->JPEGQuality_Slider, &QSlider::valueChanged, this, &MainWindow::onJPEGQualityValueChanged);
    connect(ui->JPEGQuality_SpinBox, &QSpinBox::valueChanged, this, &MainWindow::onJPEGQualityValueChanged);
    connect(ui->PNGQuality_Slider, &QSlider::valueChanged, this, &MainWindow::onPNGQualityValueChanged);
    connect(ui->PNGQuality_SpinBox, &QSpinBox::valueChanged, this, &MainWindow::onPNGQualityValueChanged);
    connect(ui->WebPQuality_Slider, &QSlider::valueChanged, this, &MainWindow::onWebPQualityValueChanged);
    connect(ui->WebPQuality_SpinBox, &QSpinBox::valueChanged, this, &MainWindow::onWebPQualityValueChanged);
    connect(ui->JPEGChromaSubsampling_ComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::onJPEGChromaSubsamplingChanged);
    connect(ui->PNGOptimizationLevel_Slider, &QSlider::valueChanged, this, &MainWindow::onPNGOptimizationLevelChanged);
    connect(ui->PNGOptimizationLevel_SpinBox, &QSpinBox::valueChanged, this, &MainWindow::onPNGOptimizationLevelChanged);

    connect(ui->TIFFCompressionMethod_ComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::onTIFFCompressionMethodChanged);
    connect(ui->TIFFDeflateLevel_Slider, &QSlider::valueChanged, this, &MainWindow::onTIFFDeflateLevelChanged);
    this->readSettings();

    connect(ui->format_ComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::outputFormatIndexChanged);

    this->on_keepAspectRatio_CheckBox_toggled(ui->keepAspectRatio_CheckBox->isChecked());
    this->on_doNotEnlarge_CheckBox_toggled(ui->doNotEnlarge_CheckBox->isChecked());
    this->on_keepAspectRatio_CheckBox_toggled(ui->keepAspectRatio_CheckBox->isChecked());
    this->on_sameOutputFolderAsInput_CheckBox_toggled(ui->sameOutputFolderAsInput_CheckBox->isChecked());
    this->moveOriginalFileToggled(ui->moveOriginalFile_CheckBox->isChecked());
    this->on_fitTo_ComboBox_currentIndexChanged(ui->fitTo_ComboBox->currentIndex());
    this->toggleLosslessWarningVisible();

    ui->actionToolbarIcons_only->setChecked(ui->toolBar->toolButtonStyle() == Qt::ToolButtonIconOnly && !ui->toolBar->isHidden());
    ui->actionToolbarIcons_and_Text->setChecked(ui->toolBar->toolButtonStyle() == Qt::ToolButtonTextUnderIcon && !ui->toolBar->isHidden());
    ui->actionToolbarHide->setChecked(ui->toolBar->isHidden());

    QSettings settings;
    if (settings.value("preferences/general/send_usage_reports", true).toBool()) {
        if (!settings.contains("access_token")) {
            this->networkOperations->requestToken();
        } else {
            this->networkOperations->updateSystemInfo();
        }
    }

    QCommandLineParser commandLineParser;
    commandLineParser.process(QApplication::arguments());
    const QStringList args = commandLineParser.positionalArguments();
    if (!args.isEmpty()) {
        this->importFromArgs(args);
    }

    QImageReader::setAllocationLimit(1024);
}

MainWindow::~MainWindow()
{
#ifdef Q_OS_WIN
    win_sparkle_cleanup();
#endif

    delete proxyModel;
    delete cImageModel;
    delete aboutDialog;
    delete keepDatesButtonGroup;
    delete compressionWatcher;
    delete networkOperations;
    delete previewWatcher;
    delete trayIcon;
    delete translator;
    delete ui;
}

void MainWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);

    this->initUpdater();
}

void MainWindow::initStatusBar()
{
    ui->statusbar->addPermanentWidget(ui->losslessWarning_Button);
    ui->statusbar->addPermanentWidget(ui->compressionProgress_Label);
    ui->statusbar->addPermanentWidget(ui->compression_ProgressBar);
    ui->statusbar->addPermanentWidget(ui->cancelCompression_Button);

    ui->losslessWarning_Button->hide();
    ui->cancelCompression_Button->hide();
    ui->compression_ProgressBar->hide();
    ui->compressionProgress_Label->hide();
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

void MainWindow::initTrayIconContextMenu()
{
    auto* exitAction = new QAction(tr("Exit"));
    this->trayIconContextMenu->addAction(ui->actionShow);
    this->trayIconContextMenu->addAction(exitAction);
    connect(exitAction, &QAction::triggered, ui->actionExit, &QAction::triggered);
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

void MainWindow::initTrayIcon()
{
#ifdef Q_OS_MAC
    QIcon icon = QIcon(":/icons/logo_mono.png");
    icon.setIsMask(true);
#else
    QIcon icon = QIcon(":/icons/logo.png");
#endif

    this->trayIcon->setIcon(icon);
    this->trayIcon->setContextMenu(this->trayIconContextMenu);
    connect(this->trayIcon, &QSystemTrayIcon::messageClicked, this, &MainWindow::showNormal);
    this->trayIcon->show();
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
        QIODevice::tr("Image Files") + " (*.jpg *.jpeg *.png *.webp *.tif *.tiff)");

    if (fileList.isEmpty()) {
        return;
    }

    QString baseFolder = QFileInfo(fileList.at(0)).absolutePath();
    this->lastOpenedDirectory = baseFolder;

    return this->importFiles(fileList, baseFolder);
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
    QStringList fileList = Importer::scanDirectory(directoryPath, scanSubfolders);

    if (fileList.isEmpty()) {
        return;
    }

    this->lastOpenedDirectory = directoryPath;
    return this->importFiles(fileList, directoryPath);
}

void MainWindow::writeSettings()
{
    QSettings settings;
    settings.setValue("mainwindow/geometry", this->saveGeometry());
    settings.setValue("mainwindow/window_state", this->saveState());
    settings.setValue("mainwindow/left_splitter_sizes", QVariant::fromValue<QList<int>>(ui->sidebar_HSplitter->sizes()));
    settings.setValue("mainwindow/previews_visible", ui->actionShow_previews->isChecked());
    settings.setValue("mainwindow/auto_preview", ui->actionAuto_preview->isChecked());

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
    settings.setValue("mainwindow/compression/jpeg_options_visible", ui->JPEGToggle_ToolButton->contentVisible());
    settings.setValue("mainwindow/compression/png_options_visible", ui->PNGToggle_ToolButton->contentVisible());
    settings.setValue("mainwindow/compression/webp_options_visible", ui->WebPToggle_ToolButton->contentVisible());
    settings.setValue("mainwindow/compression/tiff_options_visible", ui->TIFFToggle_ToolButton->contentVisible());

    settings.setValue("compression_options/compression/mode", ui->compressionMode_ComboBox->currentIndex());
    settings.setValue("compression_options/compression/lossless", ui->lossless_CheckBox->isChecked());
    settings.setValue("compression_options/compression/keep_metadata", ui->keepMetadata_CheckBox->isChecked());
    settings.setValue("compression_options/compression/keep_structure", ui->keepStructure_CheckBox->isChecked());
    settings.setValue("compression_options/compression/jpeg_quality", ui->JPEGQuality_Slider->value());
    settings.setValue("compression_options/compression/jpeg_chroma_subsampling", ui->JPEGChromaSubsampling_ComboBox->currentData(Qt::UserRole));
    settings.setValue("compression_options/compression/png_quality", ui->PNGQuality_Slider->value());
    settings.setValue("compression_options/compression/png_optimization_level", ui->PNGOptimizationLevel_Slider->value());
    settings.setValue("compression_options/compression/webp_quality", ui->WebPQuality_Slider->value());
    settings.setValue("compression_options/compression/tiff_method", ui->TIFFCompressionMethod_ComboBox->currentIndex());
    settings.setValue("compression_options/compression/tiff_deflate_level", ui->TIFFDeflateLevel_Slider->value());
    settings.setValue("compression_options/compression/max_output_size", ui->maxOutputSize_SpinBox->value());
    settings.setValue("compression_options/compression/max_output_size_unit", ui->maxOutputSizeUnit_ComboBox->currentIndex());

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
    settings.setValue("compression_options/output/skip_if_bigger", ui->skipIfBigger_CheckBox->isChecked());
    settings.setValue("compression_options/output/keep_dates", ui->keepDates_CheckBox->checkState());
    settings.setValue("compression_options/output/keep_creation_date", ui->keepCreationDate_CheckBox->isChecked());
    settings.setValue("compression_options/output/keep_last_modified_date", ui->keepLastModifiedDate_CheckBox->isChecked());
    settings.setValue("compression_options/output/keep_last_access_date", ui->keepLastAccessDate_CheckBox->isChecked());
    settings.setValue("compression_options/output/format", ui->format_ComboBox->currentIndex());
    settings.setValue("compression_options/output/move_original_file", ui->moveOriginalFile_CheckBox->isChecked());
    settings.setValue("compression_options/output/move_original_file_destination", ui->moveOriginalFile_ComboBox->currentIndex());

    settings.setValue("extra/last_opened_directory", this->lastOpenedDirectory);
}

void MainWindow::writeSetting(const QString& key, const QVariant& value)
{
    QSettings().setValue(key, value);
}

void MainWindow::readSettings()
{
    QSettings settings;
    this->restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
    this->restoreState(settings.value("mainwindow/window_state").toByteArray());
    ui->sidebar_HSplitter->setSizes(settings.value("mainwindow/left_splitter_sizes", QVariant::fromValue<QList<int>>({ 600, 1 })).value<QList<int>>());
    ui->main_VSplitter->setSizes(settings.value("mainwindow/main_splitter_sizes", QVariant::fromValue<QList<int>>({ 500, 250 })).value<QList<int>>());
    ui->actionShow_previews->setChecked(settings.value("mainwindow/previews_visible", true).toBool());
    ui->actionAuto_preview->setChecked(settings.value("mainwindow/auto_preview", false).toBool());
    ui->toolBar->setVisible(settings.value("mainwindow/toolbar/visible", true).toBool());
    ui->toolBar->setToolButtonStyle(settings.value("mainwindow/toolbar/button_style", Qt::ToolButtonIconOnly).value<Qt::ToolButtonStyle>());
    ui->JPEGToggle_ToolButton->setContentVisible(settings.value("mainwindow/compression/jpeg_options_visible", true).toBool());
    ui->PNGToggle_ToolButton->setContentVisible(settings.value("mainwindow/compression/png_options_visible", true).toBool());
    ui->WebPToggle_ToolButton->setContentVisible(settings.value("mainwindow/compression/webp_options_visible", true).toBool());
    ui->TIFFToggle_ToolButton->setContentVisible(settings.value("mainwindow/compression/tiff_options_visible", true).toBool());

    ui->compressionMode_ComboBox->setCurrentIndex(settings.value("compression_options/compression/mode", 0).toInt());
    ui->lossless_CheckBox->setChecked(settings.value("compression_options/compression/lossless", false).toBool());
    ui->keepMetadata_CheckBox->setChecked(settings.value("compression_options/compression/keep_metadata", true).toBool());
    ui->keepStructure_CheckBox->setChecked(settings.value("compression_options/compression/keep_structure", false).toBool());
    ui->JPEGQuality_Slider->setValue(settings.value("compression_options/compression/jpeg_quality", 80).toInt());
    ui->JPEGChromaSubsampling_ComboBox->setCurrentIndex(static_cast<int>(getChromaSubsamplingOptions().keys().indexOf(settings.value("compression_options/compression/jpeg_chroma_subsampling", 0).toInt())));
    ui->PNGQuality_SpinBox->setValue(settings.value("compression_options/compression/png_quality", 80).toInt());
    ui->PNGOptimizationLevel_Slider->setValue(settings.value("compression_options/compression/png_optimization_level", 3).toInt());
    ui->JPEGQuality_SpinBox->setValue(settings.value("compression_options/compression/jpeg_quality", 80).toInt());
    ui->WebPQuality_SpinBox->setValue(settings.value("compression_options/compression/webp_quality", 60).toInt());
    ui->TIFFCompressionMethod_ComboBox->setCurrentIndex(settings.value("compression_options/compression/tiff_method", 1).toInt());
    ui->TIFFDeflateLevel_Slider->setValue(settings.value("compression_options/compression/tiff_deflate_level", 2).toInt());
    ui->maxOutputSize_SpinBox->setValue(settings.value("compression_options/compression/max_output_size", 500).toInt());
    ui->maxOutputSizeUnit_ComboBox->setCurrentIndex(settings.value("compression_options/compression/max_output_size_unit", 0).toInt());

    ui->fitTo_ComboBox->setCurrentIndex(settings.value("compression_options/resize/fit_to", 0).toInt());
    ui->width_SpinBox->setValue(settings.value("compression_options/resize/width", 1000).toInt());
    ui->height_SpinBox->setValue(settings.value("compression_options/resize/height", 1000).toInt());
    ui->edge_SpinBox->setValue(settings.value("compression_options/resize/size", 1000).toInt());
    ui->keepAspectRatio_CheckBox->setChecked(settings.value("compression_options/resize/keep_aspect_ratio", false).toBool());
    ui->doNotEnlarge_CheckBox->setChecked(settings.value("compression_options/resize/do_not_enlarge", false).toBool());

    ui->outputFolder_LineEdit->setText(settings.value("compression_options/output/output_folder", "").toString());
    ui->outputSuffix_LineEdit->setText(settings.value("compression_options/output/output_suffix", "").toString());
    ui->sameOutputFolderAsInput_CheckBox->setChecked(settings.value("compression_options/output/same_folder_as_input", false).toBool());
    ui->skipIfBigger_CheckBox->setChecked(settings.value("compression_options/output/skip_if_bigger", true).toBool());
    ui->keepDates_CheckBox->setCheckState(settings.value("compression_options/output/keep_dates", Qt::Unchecked).value<Qt::CheckState>());
    ui->keepCreationDate_CheckBox->setChecked(settings.value("compression_options/output/keep_creation_date", false).toBool());
    ui->keepLastModifiedDate_CheckBox->setChecked(settings.value("compression_options/output/keep_last_modified_date", false).toBool());
    ui->keepLastAccessDate_CheckBox->setChecked(settings.value("compression_options/output/keep_last_access_date", false).toBool());
    ui->format_ComboBox->setCurrentIndex(settings.value("compression_options/output/format", 0).toInt());
    ui->moveOriginalFile_CheckBox->setChecked(settings.value("compression_options/output/move_original_file", settings.value("compression_options/output/move_original_to_trash", false).toBool()).toBool()); // Trying to get legacy value
    ui->moveOriginalFile_ComboBox->setCurrentIndex(settings.value("compression_options/output/move_original_file_destination", 0).toInt());

    this->lastOpenedDirectory = settings.value("extra/last_opened_directory", QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0)).toString();
}

void MainWindow::previewImage(const QModelIndex& imageIndex, bool forceRuntimePreview)
{
    if (this->previewWatcher->isRunning()) {
        this->previewWatcher->cancel();
        this->previewWatcher->waitForFinished();
    }
    QSettings settings;
    if (!settings.value("mainwindow/previews_visible", false).toBool()) {
        return;
    }
    ui->preview_GraphicsView->removePixmap();
    ui->previewCompressed_GraphicsView->removePixmap();

    ui->preview_GraphicsView->resetScaleFactor();
    ui->preview_GraphicsView->setLoading(true);
    ui->originalImageSize_Label->setLoading(true);
    ui->previewCompressed_GraphicsView->resetScaleFactor();

    ui->actionPreview->setEnabled(false);

    CImage* cImage = this->cImageModel->getRootItem()->children().at(imageIndex.row())->getCImage();
    QString imageToBePreviewed = forceRuntimePreview ? cImage->getTemporaryPreviewFullPath() : cImage->getCompressedFullPath();
    QList<std::pair<QString, bool>> images;
    images.append(std::pair<QString, bool>(cImage->getFullPath(), false));

    // TODO Manage failure better
    std::function<ImagePreview(std::pair<QString, bool>)> loadPixmap = [this, forceRuntimePreview, cImage](std::pair<QString, bool> pair) {
        QString previewFullPath = pair.first;
        ImagePreview imagePreview;
        bool isOnFlyPreview = false;
        if (pair.second && forceRuntimePreview && !QFileInfo::exists(previewFullPath)) {
            isOnFlyPreview = true;
            bool result = cImage->preview(this->getCompressionOptions(this->importedFilesRootFolder));
            if (!result) {
                previewFullPath = cImage->getCompressedFullPath();
            }
        }
        auto* imageReader = new QImageReader(previewFullPath);
        imageReader->setAutoTransform(true);
        QPixmap image = QPixmap::fromImageReader(imageReader);
        imagePreview.image = image;
        imagePreview.fileInfo = QFileInfo(previewFullPath);
        imagePreview.originalSize = cImage->getOriginalSize();
        imagePreview.isOnFlyPreview = isOnFlyPreview;
        imagePreview.format = QString(imageReader->format()).toUpper();
        delete imageReader; // Or the file will be opened forever
        return imagePreview;
    };

    if (!cImage->getCompressedFullPath().isEmpty() || forceRuntimePreview) {
        images.append(std::pair<QString, bool>(imageToBePreviewed, true));
        ui->previewCompressed_GraphicsView->setLoading(true);
        ui->compressedImageSize_Label->setLoading(true);
    } else {
        ui->previewCompressed_GraphicsView->setLoading(false);
        ui->compressedImageSize_Label->setLoading(false);
        ui->previewCompressed_GraphicsView->scene()->setSceneRect(ui->previewCompressed_GraphicsView->scene()->itemsBoundingRect());
        ui->previewCompressed_GraphicsView->fitInView(ui->previewCompressed_GraphicsView->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
        ui->previewCompressed_GraphicsView->show();
    }
    ui->preview_GraphicsView->setZoomEnabled(false);
    ui->previewCompressed_GraphicsView->setZoomEnabled(false);

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
            qWarning() << fileList.at(i) << "is not supported. Error:" << e.what();
        } catch (ImageTooBigException& e) {
            qWarning() << fileList.at(i) << "is too big. Error:" << e.what();
        }

        progressDialog.setValue(i);
    }

    if (!list.isEmpty() && listLength > 0) {
        this->updateFolderMap(std::move(baseFolder), (int)list.count());
        QString rootFolder = Importer::getRootFolder(this->folderMap.keys());
        this->cImageModel->appendItems(list, rootFolder);
        this->importedFilesRootFolder = rootFolder;
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

    for (long long i = indexes.count() / columnCount; i > 0; i--) {
        auto currentIndex = this->proxyModel->mapToSource(indexes.at(i));
        auto indexRow = currentIndex.row();
        auto indexParent = currentIndex.parent();
        this->updateFolderMap(this->cImageModel->getRootItem()->children().at(indexRow)->getCImage()->getFullPath(), -1);
        this->cImageModel->removeRows(indexRow, 1, indexParent);
    }
    this->previewWatcher->cancel();
    ui->preview_GraphicsView->removePixmap();
    ui->previewCompressed_GraphicsView->removePixmap();
    ui->preview_GraphicsView->scene()->setSceneRect(ui->preview_GraphicsView->scene()->itemsBoundingRect());
    ui->previewCompressed_GraphicsView->scene()->setSceneRect(ui->previewCompressed_GraphicsView->scene()->itemsBoundingRect());
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
        msgBox.setText(tr("Please select an output folder first"));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setButtonText(QMessageBox::Ok, tr("Ok"));
        msgBox.exec();
        return;
    }

    QString rootFolder = Importer::getRootFolder(this->folderMap.keys());

    bool skipDialogs = settings.value("preferences/general/skip_compression_dialogs").toBool();
    bool overwriteWarningFlag = (ui->sameOutputFolderAsInput_CheckBox->isChecked() && ui->outputSuffix_LineEdit->text().isEmpty())
        || rootFolder == ui->outputFolder_LineEdit->text();

    if (overwriteWarningFlag && !skipDialogs) {
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
    int totalImages = this->cImageModel->getRootItem()->childCount();

    if (totalImages == 0) {
        return;
    }

    if (!settings.value("preferences/general/multithreading", true).toBool()) {
        QThreadPool::globalInstance()->setMaxThreadCount(1);
    } else {
        int maxThreads = settings.value("preferences/general/multithreading_max_threads", QThread::idealThreadCount()).toInt();
        QThreadPool::globalInstance()->setMaxThreadCount(maxThreads);
    }
    qDebug() << settings.value("preferences/general/threads_priority", QThread::NormalPriority).value<QThread::Priority>();
    QThreadPool::globalInstance()->setThreadPriority(settings.value("preferences/general/threads_priority", QThread::NormalPriority).value<QThread::Priority>());

    this->compressionWatcher = new QFutureWatcher<void>();
    connect(this->compressionWatcher, &QFutureWatcherBase::finished, this, &MainWindow::compressionFinished);
    connect(this->compressionWatcher, &QFutureWatcherBase::progressValueChanged, this->cImageModel, &CImageTreeModel::emitDataChanged);
    connect(this->compressionWatcher, &QFutureWatcherBase::progressValueChanged, this, &MainWindow::updateCompressionProgressLabel);

    ui->cancelCompression_Button->show();
    ui->compression_ProgressBar->show();
    ui->compressionProgress_Label->show();

    ui->cancelCompression_Button->setEnabled(true);
    ui->compressionProgress_Label->setText(tr("Compressing...") + QString(" (%1/%2)").arg("0", QString::number(totalImages)));
    ui->compression_ProgressBar->setMinimum(0);
    ui->compression_ProgressBar->setMaximum(totalImages);
    connect(this->compressionWatcher, &QFutureWatcherBase::progressValueChanged, ui->compression_ProgressBar, &QProgressBar::setValue);
    connect(ui->cancelCompression_Button, &QPushButton::clicked, this, &MainWindow::compressionCanceled);

    CompressionOptions compressionOptions = this->getCompressionOptions(rootFolder);

    this->compressionWatcher->setFuture(this->cImageModel->getRootItem()->compress(compressionOptions));
    compressionSummary.totalImages = this->cImageModel->rowCount();
    compressionSummary.totalUncompressedSize = this->cImageModel->originalItemsSize();
    compressionSummary.totalCompressedSize = 0;
    compressionSummary.elapsedTime = 0;

    this->toggleUIEnabled(false);

    compressionTimer.start();
}

CompressionOptions MainWindow::getCompressionOptions(QString rootFolder)
{
    FileDatesOutputOption datesMap = {
        ui->keepCreationDate_CheckBox->isChecked(),
        ui->keepLastModifiedDate_CheckBox->isChecked(),
        ui->keepLastAccessDate_CheckBox->isChecked()
    };

    MaxOutputSize maxOutputSize {
        static_cast<MaxOutputSizeUnit>(ui->maxOutputSizeUnit_ComboBox->currentIndex()),
        static_cast<size_t>(ui->maxOutputSize_SpinBox->value())
    };

    CompressionOptions compressionOptions = {
        ui->outputFolder_LineEdit->text(),
        rootFolder,
        ui->outputSuffix_LineEdit->text(),
        ui->format_ComboBox->currentIndex(),
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
        ui->skipIfBigger_CheckBox->isChecked(),
        ui->moveOriginalFile_CheckBox->isChecked(),
        ui->moveOriginalFile_ComboBox->currentIndex(),
        qBound(ui->JPEGQuality_Slider->value(), 1, 100),
        ui->JPEGChromaSubsampling_ComboBox->currentData(Qt::UserRole).toInt(),
        qBound(ui->PNGQuality_Slider->value(), 0, 100),
        qBound(ui->PNGOptimizationLevel_Slider->value(), 1, 6),
        qBound(ui->WebPQuality_Slider->value(), 1, 100),
        ui->TIFFCompressionMethod_ComboBox->currentIndex(),
        qBound(ui->TIFFDeflateLevel_Slider->value() * 3, 1, 9),
        ui->keepDates_CheckBox->checkState() != Qt::Unchecked,
        datesMap,
        static_cast<CompressionMode>(ui->compressionMode_ComboBox->currentIndex()),
        maxOutputSize,
    };

    return compressionOptions;
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
    this->clearCache();
    this->previewWatcher->waitForFinished();
    qInfo() << "---- Closing application ----";
    Logger::closeLogFile();
    Logger::cleanOldLogs();
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
    ui->originalImageSize_Label->clear();
    ui->compressedImageSize_Label->clear();
    ui->actionPreview->setEnabled(this->selectedCount == 1 && !this->previewWatcher->isRunning());
    if (this->isItemRemovalRunning) {
        return;
    }
    ui->actionRemove->setDisabled(this->selectedCount == 0);
    ui->removeFiles_Button->setDisabled(this->selectedCount == 0);
    ui->actionShow_original_in_file_manager->setEnabled(this->selectedCount == 1);
    ui->actionShow_compressed_in_file_manager->setEnabled(this->selectedCount == 1);

    if (this->selectedCount == 0) {
        ui->preview_GraphicsView->removePixmap();
        ui->previewCompressed_GraphicsView->removePixmap();
        return;
    }

    auto currentIndex = this->selectedIndexes.at(0);

    if (currentIndex.row() == -1) {
        return;
    }

    bool autoPreview = QSettings().value("mainwindow/auto_preview", false).toBool();
    this->previewImage(this->proxyModel->mapToSource(currentIndex), autoPreview);
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

    ui->cancelCompression_Button->hide();
    ui->compression_ProgressBar->hide();
    ui->compressionProgress_Label->hide();
    this->toggleUIEnabled(true);

    PostCompressionAction postCompressionAction = static_cast<PostCompressionAction>(settings.value("preferences/general/post_compression_action", 0).toInt());
    if (postCompressionAction != PostCompressionAction::NO_ACTION) {
        PostCompressionActions::runAction(postCompressionAction, ui->outputFolder_LineEdit->text());
        return;
    }

    this->trayIcon->showMessage(title, tr("You just saved %1!").arg(saved), QSystemTrayIcon::NoIcon);
    if (!settings.value("preferences/general/skip_compression_dialogs").toBool()) {
        QCaesiumMessageBox compressionSummaryDialog;
        compressionSummaryDialog.setText(title);
        compressionSummaryDialog.setInformativeText(tr("Total files: %1\nOriginal size: %2\nCompressed size: %3\nSaved: %4 (%5%)")
                                                        .arg(QString::number(compressionSummary.totalImages),
                                                            toHumanSize(compressionSummary.totalUncompressedSize),
                                                            toHumanSize(compressionSummary.totalCompressedSize),
                                                            saved,
                                                            savedPerc));
        compressionSummaryDialog.setStandardButtons(QMessageBox::Ok);
        compressionSummaryDialog.setButtonText(QMessageBox::Ok, tr("Ok"));
        compressionSummaryDialog.exec();
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
    QString baseFolder = Importer::getRootFolder(filePaths);
    this->importFiles(filePaths, baseFolder);
}

void MainWindow::on_fitTo_ComboBox_currentIndexChanged(int index)
{
    QSettings settings;
    switch (index) {
    default:
    case ResizeMode::NO_RESIZE:
        ui->resize_Frame->setHidden(true);
        break;
    case ResizeMode::DIMENSIONS:
        ui->resize_Frame->setHidden(false);
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
        ui->resize_Frame->setHidden(false);
        ui->edge_Label->hide();
        ui->edge_SpinBox->hide();
        ui->width_Label->show();
        ui->width_SpinBox->show();
        ui->width_SpinBox->setSuffix(tr("%"));
        ui->width_SpinBox->setMaximum(ui->doNotEnlarge_CheckBox->isChecked() ? 100 : 999);
        ui->height_Label->show();
        ui->height_SpinBox->show();
        ui->height_SpinBox->setSuffix(tr("%"));
        ui->height_SpinBox->setMaximum(ui->doNotEnlarge_CheckBox->isChecked() ? 100 : 999);
        ui->keepAspectRatio_CheckBox->setEnabled(true);
        break;
    case ResizeMode::SHORT_EDGE:
    case ResizeMode::LONG_EDGE:
        ui->resize_Frame->setHidden(false);
        ui->edge_Label->show();
        ui->edge_SpinBox->show();
        ui->width_Label->hide();
        ui->width_SpinBox->hide();
        ui->height_Label->hide();
        ui->height_SpinBox->hide();
        ui->keepAspectRatio_CheckBox->setDisabled(true);
        break;
    case ResizeMode::FIXED_WIDTH:
        ui->resize_Frame->setHidden(false);
        ui->edge_Label->hide();
        ui->edge_SpinBox->hide();
        ui->width_Label->show();
        ui->width_SpinBox->show();
        ui->width_SpinBox->setSuffix(tr("px"));
        ui->height_Label->hide();
        ui->height_SpinBox->hide();
        ui->height_SpinBox->setSuffix(tr("px"));
        ui->keepAspectRatio_CheckBox->setEnabled(false);
        break;
    case ResizeMode::FIXED_HEIGHT:
        ui->resize_Frame->setHidden(false);
        ui->edge_Label->hide();
        ui->edge_SpinBox->hide();
        ui->width_Label->hide();
        ui->width_SpinBox->hide();
        ui->width_SpinBox->setSuffix(tr("px"));
        ui->height_Label->show();
        ui->height_SpinBox->show();
        ui->height_SpinBox->setSuffix(tr("px"));
        ui->keepAspectRatio_CheckBox->setEnabled(false);
        break;
    }

    this->writeSetting("compression_options/resize/fit_to", index);
    this->toggleLosslessWarningVisible();
}

void MainWindow::on_width_SpinBox_valueChanged(int value)
{
    if (ui->fitTo_ComboBox->currentIndex() == ResizeMode::PERCENTAGE && ui->keepAspectRatio_CheckBox->isChecked()) {
        ui->height_SpinBox->setValue(value);
    }
}

void MainWindow::on_height_SpinBox_valueChanged(int value)
{
    if (ui->fitTo_ComboBox->currentIndex() == ResizeMode::PERCENTAGE && ui->keepAspectRatio_CheckBox->isChecked()) {
        ui->width_SpinBox->setValue(value);
    }
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

    ui->JPEGOptions_Frame->setEnabled(!checked);
    ui->WebPOptions_Frame->setEnabled(!checked);

    ui->PNGQuality_Label->setEnabled(!checked);
    ui->PNGQuality_SpinBox->setEnabled(!checked);
    ui->PNGQuality_Slider->setEnabled(!checked);
    ui->PNGOptimizationLevel_Label->setEnabled(checked);
    ui->PNGOptimizationLevel_SpinBox->setEnabled(checked);
    ui->PNGOptimizationLevel_Slider->setEnabled(checked);

    this->toggleLosslessWarningVisible();
}

void MainWindow::on_keepMetadata_CheckBox_toggled(bool checked)
{
    this->writeSetting("compression_options/compression/keep_metadata", checked);
}

void MainWindow::onJPEGQualityValueChanged(int value)
{
    if (ui->JPEGQuality_Slider->value() != value) {
        ui->JPEGQuality_Slider->setValue(value);
    }

    if (ui->JPEGQuality_SpinBox->value() != value) {
        ui->JPEGQuality_SpinBox->setValue(value);
    }
    this->writeSetting("compression_options/compression/jpeg_quality", value);
}

void MainWindow::onPNGQualityValueChanged(int value)
{
    if (ui->PNGQuality_SpinBox->value() != value) {
        ui->PNGQuality_SpinBox->setValue(value);
    }
    if (ui->PNGQuality_Slider->value() != value) {
        ui->PNGQuality_Slider->setValue(value);
    }
    this->writeSetting("compression_options/compression/png_quality", value);
}

void MainWindow::onWebPQualityValueChanged(int value)
{
    if (ui->WebPQuality_SpinBox->value() != value) {
        ui->WebPQuality_SpinBox->setValue(value);
    }
    if (ui->WebPQuality_Slider->value() != value) {
        ui->WebPQuality_Slider->setValue(value);
    }
    this->writeSetting("compression_options/compression/webp_quality", value);
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
    bool isPortable = false;
#ifdef IS_PORTABLE
    isPortable = true;
#endif
#ifdef Q_OS_MAC
    CocoaInitializer initializer;
    auto updater = new SparkleAutoUpdater("https://saerasoft.com/repository/com.saerasoft.caesium/osx/appcast.xml");
    updater->setCheckForUpdatesAutomatically(settings.value("preferences/general/check_updates_at_startup", true).toBool());
    if (settings.value("preferences/general/check_updates_at_startup", true).toBool()) {
        updater->checkForUpdates();
    }
#endif

#ifdef Q_OS_WIN
    if (isPortable) {
        return;
    }

    QString locale = LanguageManager::getLocaleFromPreferences(settings.value("preferences/language/locale", "default"));
    if (locale != "default") {
        win_sparkle_set_lang(locale.replace('_', '-').toUtf8().constData());
    }
    win_sparkle_set_appcast_url("https://saerasoft.com/repository/com.saerasoft.caesium/win/appcast.xml");
    win_sparkle_init();

    if (settings.value("preferences/general/check_updates_at_startup", true).toBool()) {
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
        if (this->selectedCount == 1) {
            bool autoPreview = QSettings().value("mainwindow/auto_preview", false).toBool();
            this->previewImage(this->proxyModel->mapToSource(this->selectedIndexes.at(0)), autoPreview);
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
    ImagePreview imagePreview = previewWatcher->resultAt(index);
    if (index == 0) {
        ui->preview_GraphicsView->setLoading(false);
        ui->originalImageSize_Label->setLoading(false);
        ui->preview_GraphicsView->showPixmap(imagePreview.image);
        ui->preview_GraphicsView->fitInView(ui->preview_GraphicsView->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
        ui->preview_GraphicsView->show();
        if (imagePreview.fileInfo.exists()) {
            ui->originalImageSize_Label->setText(QString("%1 %2").arg(toHumanSize((double)imagePreview.fileInfo.size()), imagePreview.format));
        } else {
            ui->originalImageSize_Label->setText(tr("File not found"));
        }
    }

    if (index == 1) {
        QString labelTextPrefix = tr("File not found");
        if (imagePreview.fileInfo.exists()) {
            auto originalSize = (double)imagePreview.originalSize;
            auto currentSize = (double)imagePreview.fileInfo.size();
            QString icon = "=";
            QString color = "#14b8a6";
            if (currentSize < originalSize) {
                icon = "↓";
                color = "#22c55e";
            } else if (currentSize > originalSize) {
                icon = "↑";
                color = "#ef4444";
            }
            QString ratio = QString::number(round(-100 + (currentSize / originalSize * 100))) + "%";
            labelTextPrefix = QString("<span style=\" color:%1;\">%2</span> %3 (%4) %5").arg(color, icon, toHumanSize(currentSize), ratio, imagePreview.format);
            if (imagePreview.isOnFlyPreview) {
                labelTextPrefix += " (" + tr("Preview") + ")";
            }
        }
        ui->previewCompressed_GraphicsView->setLoading(false);
        ui->compressedImageSize_Label->setLoading(false);
        ui->previewCompressed_GraphicsView->showPixmap(imagePreview.image);
        ui->compressedImageSize_Label->setText(labelTextPrefix);
        ui->previewCompressed_GraphicsView->fitInView(ui->previewCompressed_GraphicsView->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
        ui->previewCompressed_GraphicsView->show();
    }
}
void MainWindow::compressionCanceled()
{
    qInfo() << "Compression canceled by user.";
    ui->cancelCompression_Button->setEnabled(false);
    ui->compressionProgress_Label->setText(tr("Finishing..."));
    ui->compression_ProgressBar->setMinimum(0);
    ui->compression_ProgressBar->setMaximum(0);
    ui->compression_ProgressBar->setValue(0);
    this->cImageModel->getRootItem()->setCompressionCanceled(true);
    this->compressionWatcher->cancel();
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

void MainWindow::toggleUIEnabled(bool enabled)
{
    ui->toolBar->setEnabled(enabled);
    ui->parameters_TabWidget->setEnabled(enabled);
    ui->listActions_Frame->setEnabled(enabled);
    listContextMenu->setEnabled(enabled);
    ui->menuBar->setEnabled(enabled);
}

void MainWindow::updateCompressionProgressLabel(int value)
{
    ui->compressionProgress_Label->setText(tr("Compressing...") + QString(" (%1/%2)").arg(QString::number(value), QString::number(ui->compression_ProgressBar->maximum())));
}

void MainWindow::on_actionAuto_preview_toggled(bool toggled)
{
    this->writeSetting("mainwindow/auto_preview", toggled);
}

void MainWindow::previewFinished()
{
    ui->preview_GraphicsView->setZoomEnabled(true);
    ui->previewCompressed_GraphicsView->setZoomEnabled(true);
    ui->actionPreview->setEnabled(this->selectedCount == 1);
}

void MainWindow::clearCache()
{
    QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    if (!cacheDir.exists()) {
        return;
    }

    foreach (QString cacheFile, cacheDir.entryList()) {
        cacheDir.remove(cacheFile);
    }
}

void MainWindow::previewCanceled()
{
    ui->preview_GraphicsView->setLoading(false);
    ui->originalImageSize_Label->setLoading(false);
    ui->previewCompressed_GraphicsView->setLoading(false);
    ui->compressedImageSize_Label->setLoading(false);
}

void MainWindow::on_actionPreview_triggered()
{
    if (this->selectedCount != 1) {
        return;
    }

    auto currentIndex = this->selectedIndexes.at(0);

    if (currentIndex.row() == -1) {
        return;
    }

    this->previewImage(this->proxyModel->mapToSource(currentIndex), true);
}

void MainWindow::on_skipIfBigger_CheckBox_toggled(bool checked)
{
    this->writeSetting("compression_options/output/skip_if_bigger", checked);
}

void MainWindow::outputFormatIndexChanged(int index)
{
    this->writeSetting("compression_options/output/format", index);
    this->toggleLosslessWarningVisible();
}

void MainWindow::importFromArgs(const QStringList& args)
{
    bool scanSubfolders = QSettings().value("preferences/general/import_subfolders", true).toBool();
    QStringList filesList = Importer::scanList(args, scanSubfolders);
    if (filesList.isEmpty()) {
        return;
    }

    ImportFromArgsMethod argsBehaviour = static_cast<ImportFromArgsMethod>(QSettings().value("preferences/general/args_behaviour", 0).toInt());
    QString baseFolder = Importer::getRootFolder(args);
    this->importFiles(filesList, baseFolder);
    if (argsBehaviour == IMPORT_AND_COMPRESS) {
        this->startCompression();
    }
}

void MainWindow::moveOriginalFileToggled(bool checked)
{
    ui->moveOriginalFile_ComboBox->setEnabled(checked);
    this->writeSetting("compression_options/output/move_original_file", checked);
}

void MainWindow::moveOriginalFileDestinationChanged(int index)
{
    this->writeSetting("compression_options/output/move_original_file_destination", index);
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

QTranslator* MainWindow::getTranslator() const
{
    return translator;
}

void MainWindow::onMaxOutputSizeChanged(int value)
{
    this->writeSetting("compression_options/compression/max_output_size", ui->maxOutputSize_SpinBox->value());
}

void MainWindow::onMaxOutputSizeUnitChanged(int value)
{
    if (value == MaxOutputSizeUnit::MAX_OUTPUT_PERCENTAGE && ui->maxOutputSize_SpinBox->value() > 100) {
        ui->maxOutputSize_SpinBox->setValue(100);
    }
    this->writeSetting("compression_options/compression/max_output_size_unit", ui->maxOutputSizeUnit_ComboBox->currentIndex());
}

void MainWindow::toggleLosslessWarningVisible()
{
    bool showLosslessWarning = ui->lossless_CheckBox->isChecked()
        && (ui->format_ComboBox->currentIndex() != 0 || ui->fitTo_ComboBox->currentIndex() != ResizeMode::NO_RESIZE)
        && ui->compressionMode_ComboBox->currentIndex() == QUALITY;

    ui->losslessWarning_Button->setVisible(showLosslessWarning);
}

void MainWindow::onCompressionModeChanged(int value)
{
    this->writeSetting("compression_options/compression/mode", ui->compressionMode_ComboBox->currentIndex());

    this->toggleLosslessWarningVisible();
}

void MainWindow::onAdvancedImportTriggered()
{
    const auto advancedImportDialog = new AdvancedImportDialog();
    connect(advancedImportDialog, &AdvancedImportDialog::importTriggered, [this](const QStringList& fileList) {
        if (fileList.isEmpty()) {
            return;
        }
        QString baseFolder = Importer::getRootFolder(fileList);
        this->importFiles(fileList, baseFolder);
    });

    advancedImportDialog->exec();
}

void MainWindow::onPNGOptimizationLevelChanged(int value)
{
    if (ui->PNGOptimizationLevel_SpinBox->value() != value) {
        ui->PNGOptimizationLevel_SpinBox->setValue(value);
    }
    if (ui->PNGOptimizationLevel_Slider->value() != value) {
        ui->PNGOptimizationLevel_Slider->setValue(value);
    }

    this->writeSetting("compression_options/compression/png_optimization_level", value);
}

void MainWindow::onTIFFCompressionMethodChanged(int index)
{
    ui->TIFFDeflateLevelContainer_Widget->setEnabled(index == 2);
    this->writeSetting("compression_options/compression/tiff_method", index);
}

void MainWindow::onTIFFDeflateLevelChanged(int value)
{
    this->writeSetting("compression_options/compression/tiff_deflate_level", value);
}

void MainWindow::onJPEGChromaSubsamplingChanged()
{
    this->writeSetting("compression_options/compression/jpeg_chroma_subsampling", ui->JPEGChromaSubsampling_ComboBox->currentData(Qt::UserRole));
}

void MainWindow::setupChromaSubsamplingComboBox()
{
    auto chromaSubsamplings = getChromaSubsamplingOptions();
    auto iterator = QMapIterator<int, QString>(chromaSubsamplings);
    while (iterator.hasNext()) {
        auto chromaSubsamplingOption = iterator.next();
        ui->JPEGChromaSubsampling_ComboBox->addItem(chromaSubsamplingOption.value(), chromaSubsamplingOption.key());
    }
}

void MainWindow::onJPEGOptionsVisibilityChanged(bool visible)
{
    this->writeSetting("mainwindow/compression/jpeg_options_visible", visible);
}
void MainWindow::onPNGOptionsVisibilityChanged(bool visible)
{
    this->writeSetting("mainwindow/compression/png_options_visible", visible);
}
void MainWindow::onWebPOptionsVisibilityChanged(bool visible)
{
    this->writeSetting("mainwindow/compression/webp_options_visible", visible);
}
void MainWindow::onTIFFOptionsVisibilityChanged(bool visible)
{
    this->writeSetting("mainwindow/compression/tiff_options_visible", visible);
}
