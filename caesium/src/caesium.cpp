#include "caesium.h"
#include "ui_caesium.h"
#include "src/preferencesdialog.h"
#include "src/aboutdialog.h"
#include "ctreewidgetitem.h"

#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDebug>
#include <QDirIterator>
#include <QFuture>
#include <QtConcurrent>
#include <QProgressDialog>
#include <QImageReader>
#include <QStatusBar>

Caesium::Caesium(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Caesium)
{
    ui->setupUi(this);

    //Initialization
    initUI();
    initMembers();
    initConnections();
}

Caesium::~Caesium()
{
    delete originalImageWatcher;
    delete ui;
}

void Caesium::initUI()
{
    //Drop shadows
    QGraphicsDropShadowEffect *listDropShadowEffect = new QGraphicsDropShadowEffect();
    listDropShadowEffect->setBlurRadius(15);
    listDropShadowEffect->setXOffset(0);
    listDropShadowEffect->setYOffset(0);
    listDropShadowEffect->setColor(QColor(0, 0, 0, 50));
    ui->list_TreeWidget->setGraphicsEffect(listDropShadowEffect);

    QGraphicsDropShadowEffect *originalPreviewDropShadowEffect = new QGraphicsDropShadowEffect();
    originalPreviewDropShadowEffect->setBlurRadius(15);
    originalPreviewDropShadowEffect->setXOffset(0);
    originalPreviewDropShadowEffect->setYOffset(0);
    originalPreviewDropShadowEffect->setColor(QColor(0, 0, 0, 50));
    ui->originalImage_Frame->setGraphicsEffect(originalPreviewDropShadowEffect);

    QGraphicsDropShadowEffect *compressedPreviewDropShadowEffect = new QGraphicsDropShadowEffect();
    compressedPreviewDropShadowEffect->setBlurRadius(15);
    compressedPreviewDropShadowEffect->setXOffset(0);
    compressedPreviewDropShadowEffect->setYOffset(0);
    compressedPreviewDropShadowEffect->setColor(QColor(0, 0, 0, 50));
    ui->compressedImagePreview_Label->setGraphicsEffect(compressedPreviewDropShadowEffect);

    QGraphicsDropShadowEffect *optionsDropShadowEffect = new QGraphicsDropShadowEffect();
    optionsDropShadowEffect->setBlurRadius(15);
    optionsDropShadowEffect->setXOffset(0);
    optionsDropShadowEffect->setYOffset(0);
    optionsDropShadowEffect->setColor(QColor(0, 0, 0, 50));
    ui->parameters_Frame->setGraphicsEffect(optionsDropShadowEffect);

    //Splitter size
    QList<int> sizes;
    sizes << 600 << 100;
    ui->listParameters_Splitter->setSizes(sizes);

    //No blue border on focus for Mac
    ui->list_TreeWidget->setAttribute(Qt::WA_MacShowFocusRect, false);

    //Set the headers size
    //TODO This should be saved in preferences and retrived as user set
    ui->list_TreeWidget->header()->resizeSection(COLUMN_STATUS, 24);
    ui->list_TreeWidget->header()->resizeSection(COLUMN_NAME, 180);
    ui->list_TreeWidget->header()->resizeSection(COLUMN_SIZE, 80);
    ui->list_TreeWidget->header()->resizeSection(COLUMN_NEW_SIZE, 80);
    ui->list_TreeWidget->header()->resizeSection(COLUMN_RESOLUTION, 100);
    ui->list_TreeWidget->header()->resizeSection(COLUMN_NEW_RESOLUTION, 100);
    ui->list_TreeWidget->header()->resizeSection(COLUMN_SAVED, 80);
}

void Caesium::initMembers()
{
    originalImageWatcher = new QFutureWatcher<QImage>();
}

void Caesium::initConnections()
{
    qInfo() << "Initalizing connections...";
    //List changed signal
    connect(ui->list_TreeWidget, SIGNAL(itemsChanged()), this, SLOT(on_listChanged()));
    connect(ui->list_TreeWidget, SIGNAL(itemsChanged()), this, SLOT(updateStatusBar()));
}

void Caesium::toggleUI(bool enabled)
{
    ui->main_Splitter->setEnabled(enabled);
    ui->toolBar->setEnabled(enabled);
}

void Caesium::clearUI()
{
    ui->originalImagePreview_Label->clear();
    ui->compressedImagePreview_Label->clear();
}

void Caesium::closeEvent(QCloseEvent* event)
{
    bool acceptExit = true;
    QSettings settings;
    //Save window geometry
    settings.beginGroup(KEY_PREF_GROUP_GEOMETRY);
    settings.setValue(KEY_PREF_GEOMETRY_SIZE, size());
    settings.setValue(KEY_PREF_GEOMETRY_POS, pos());
    //settings.setValue(KEY_PREF_GEOMETRY_SORT_COLUMN, ui->list_TreeWidget->sortColumn());
    //settings.setValue(KEY_PREF_GEOMETRY_SORT_ORDER, ui->list_TreeWidget->header()->sortIndicatorOrder());
    settings.endGroup();

    if (settings.value(KEY_PREF_GROUP_GENERAL + KEY_PREF_GENERAL_PROMPT).value<bool>()) {
        //Display a prompt
        int res = QMessageBox::warning(this, tr("Caesium"),
                                       tr("Do you really want to exit?"),
                                       QMessageBox::Ok | QMessageBox::Cancel);
        //Exit if OK, go back if Cancel
        switch (res) {
        case QMessageBox::Cancel:
            acceptExit = false;
            break;
        default:
        case QMessageBox::Ok:
            acceptExit = true;
            break;
        }
    }
    if (acceptExit) {
        //qInfo() << "----------------- Caesium session stopped at "
        //        << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << "-----------------";
        //QFile(logPath).close();
        event->accept();
    } else {
        event->ignore();
    }
}

void Caesium::updateStatusBar()
{
    ui->statusBar->showMessage(QString::number(ui->list_TreeWidget->topLevelItemCount()) + tr(" items in list"));
}


void Caesium::on_actionAbout_Caesium_Image_Compressor_triggered()
{
    AboutDialog* aboutDialog = new AboutDialog();
    aboutDialog->show();
}

/*
 * ----
 * File import
 * ----
 */

void Caesium::on_actionAdd_files_triggered()
{
    //Generate file dialog for import and call the progress dialog indicator
    QStringList filesList = QFileDialog::getOpenFileNames(this,
                                                          tr("Import files..."),
                                                          QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0),
                                                          inputFilter);
    if (!filesList.isEmpty()) {
        importFiles(filesList);
    }
}

void Caesium::on_actionAdd_folder_triggered()
{
    qInfo() << "Importing folder...";
    QString path = QFileDialog::getExistingDirectory(this,
                                                     tr("Select a folder to import..."),
                                                     QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0),
                                                     QFileDialog::ShowDirsOnly);
    if (!path.isEmpty()) {
        importFiles(path);
    }
}

void Caesium::onImportStarted()
{
    qInfo() << "Importing files...";
    ui->statusBar->showMessage("Importing files...");
}

void Caesium::onImportFinished()
{
    qInfo() << "Import finished.";
    emit ui->list_TreeWidget->itemsChanged();
}

QStringList Caesium::scanFolder(QString path)
{
    //TODO should be in another thread
    QStringList list = QStringList();
    QSettings settings;
    bool scanSubdir = settings.value(KEY_PREF_GROUP_GENERAL + KEY_PREF_GENERAL_SUBFOLDER).value<bool>();
    //Collecting all files in folder
    if (QDir(path).exists()) {
        qInfo() << "Collecting files in" << path;
        ui->statusBar->showMessage("Collecting files in " + path);
        qInfo() << "Subfolder scanning is" << scanSubdir;
        QDirIterator it(path,
                        inputFilterList,
                        QDir::AllEntries,
                        scanSubdir ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);

        while (it.hasNext()) {
            it.next();
            list.append(it.filePath());
        }
    }

    return list;
}

void Caesium::importFiles(QStringList filesList)
{
    if (filesList.isEmpty()) {
        return;
    }
    qRegisterMetaType<QVector<int> >("QVector<int>");

    QList<CTreeWidgetItem*> itemsList;
    for (int i = 0; i < filesList.size(); i++){
        itemsList.append(new CTreeWidgetItem(ui->list_TreeWidget, filesList.at(i)));
    }
    // Create a progress dialog.
    QProgressDialog progressDialog;
    progressDialog.setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    progressDialog.setModal(true);
    progressDialog.setCancelButton(0);
    progressDialog.setLabelText(tr("Importing..."));
    progressDialog.setMinimumWidth(350);
    progressDialog.setStyleSheet("background-color:#FFFFFF");

    QFutureWatcher<void> importWatcher;

    // Create a QFutureWatcher and connect signals and slots.
    connect(&importWatcher, SIGNAL(started()), this, SLOT(onImportStarted()));
    connect(&importWatcher, SIGNAL(finished()), this, SLOT(onImportFinished()));
    connect(&importWatcher, SIGNAL(finished()), &progressDialog, SLOT(reset()));
    connect(&importWatcher, SIGNAL(progressRangeChanged(int,int)), &progressDialog, SLOT(setRange(int,int)));
    connect(&importWatcher, SIGNAL(progressValueChanged(int)),  &progressDialog, SLOT(setValue(int)));
    connect(&importWatcher, SIGNAL(progressValueChanged(int)), this, SLOT(testSlot(int)));

    //Actual function
    std::function<void(CTreeWidgetItem*)> populateItemsList = [](CTreeWidgetItem *item)
    {
        QString path = item->getPath();
        item->setImage(new CImage(path));
    };
    //Start
    importWatcher.setFuture(QtConcurrent::map(itemsList, populateItemsList));

    // Display the dialog and start the event loop.
    progressDialog.exec();
    importWatcher.waitForFinished();
}

void Caesium::importFiles(QString folder)
{
    //We are importing a folder
    QStringList filesList = scanFolder(folder);
    importFiles(filesList);
}

void Caesium::on_actionPreferences_triggered()
{
    PreferencesDialog *preferencesDialog = new PreferencesDialog();
    preferencesDialog->show();
}

//-----
void Caesium::testSlot(int value)
{

    //CTreeWidgetItem *result = importWatcher->resultAt(value);
    //result->setText(COLUMN_NAME, result->getPath());
}

/*
 * ----
 * List
 *  ----
 */


void Caesium::on_list_TreeWidget_itemSelectionChanged()
{
    bool itemsSelected = ui->list_TreeWidget->selectedItems().length() > 0;
    ui->actionPreview->setEnabled(itemsSelected);
    //Check if there's a selection
    if (itemsSelected) {
        ui->originalImagePreview_Label->clear();
        ui->compressedImagePreview_Label->clear();
        //Get the first item selected
        CTreeWidgetItem* currentItem = (CTreeWidgetItem*)ui->list_TreeWidget->selectedItems().at(0);

        //Try to load a preview
        /*previewPath = calculatePreviewHashPath(currentItem);
        qInfo() << "Checking if preview exists at" << previewPath;

        if (QFile(previewPath).exists()) {
            qDebug() << "Preview exists: true" << previewPath;
            connect(&imagePreviewWatcher, SIGNAL(resultReadyAt(int)), this, SLOT(finishPreviewLoading(int)));
            imagePreviewWatcher.setFuture(QtConcurrent::run<QImage>(this, &Caesium::loadImage, previewPath));
        }
        qInfo() << "Preview already exists:" << QFile(previewPath).exists();
        */
        if (originalImageWatcher->isRunning()) {
            originalImageWatcher->cancel();
        }

        //Connect the global watcher to the slot
        connect(originalImageWatcher, SIGNAL(resultReadyAt(int)), this, SLOT(onOriginalImageLoadingFinished(int)));
        //Run the image loader function
        originalImageWatcher->setFuture(QtConcurrent::run<QImage>(this, &Caesium::loadImage, currentItem->text(COLUMN_PATH)));
    } else {
        originalImageWatcher->cancel();
    }

    //Rule the remove button/action
    ui->actionRemove->setEnabled(itemsSelected);
}

QImage Caesium::loadImage(QString path)
{
    return QImage(path);
}

void Caesium::onOriginalImageLoadingFinished(int index)
{
    ui->originalImagePreview_Label->setMaximumWidth(ui->originalImage_Frame->width());
    ui->originalImagePreview_Label->setMaximumHeight(ui->originalImage_Frame->height());
    ui->originalImagePreview_Label->setPixmap(QPixmap::fromImage(originalImageWatcher->resultAt(index)));
}

void Caesium::on_actionClear_list_triggered()
{
    emit ui->list_TreeWidget->itemsChanged();
    clearUI();
}

void Caesium::on_listChanged()
{
    int totalItemsCount = ui->list_TreeWidget->topLevelItemCount();
    //List changed, so we need to enable the save feature
    //ui->actionSave_list->setEnabled(true);
    //ui->actionSave_list_as->setEnabled(true);

    //If the list is empty, we don't need the clear button
    ui->actionClear_list->setEnabled(totalItemsCount);

    //Nor the select all
    ui->actionSelect_All->setEnabled(totalItemsCount);

    //And we can't really compress
    ui->actionCompress->setEnabled(totalItemsCount);
}

void Caesium::on_actionRemove_triggered()
{
    int count = ui->list_TreeWidget->selectedItems().count();
    if (count == ui->list_TreeWidget->topLevelItemCount()) {
        qInfo() << "Clearing list";
        ui->list_TreeWidget->clear();
    } else {
        qInfo() << "Removing selected items";
        foreach (QTreeWidgetItem *item, ui->list_TreeWidget->selectedItems()) {
            ui->list_TreeWidget->takeTopLevelItem(ui->list_TreeWidget->indexOfTopLevelItem(item));
        }
    }
    emit ui->list_TreeWidget->itemsChanged();
    //Clear boxes
    clearUI();
}
