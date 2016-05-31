/**
 *
 * This file is part of Caesium.
 *
 * Caesium - Caesium is an image compression software aimed at helping photographers,
 * bloggers, webmasters, businesses or casual users at storing, sending and sharing digital pictures.
 *
 * Copyright (C) 2016 - Matteo Paonessa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.
 * If not, see <http://www.gnu.org/licenses/>
 *
 */


#include "caesium.h"
#include "ui_caesium.h"
#include "aboutdialog.h"
#include "utils.h"
#include "jpeg.h"
#include "cimage.h"
#include "exif.h"
#include "preferencedialog.h"
#include "networkoperations.h"
#include "qdroptreewidget.h"
#include "ctreewidgetitem.h"
#include "clist.h"
#include "png.h"

#include <QProgressDialog>
#include <QFileDialog>
#include <QStandardPaths>
#include <QGraphicsPixmapItem>
#include <QFileInfo>
#include <QtConcurrent>
#include <QFuture>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QImageReader>
#include <QSettings>
#include <QCloseEvent>
#include <QMessageBox>
#include <QDesktopServices>
#include <QDirIterator>
#include <QSizeGrip>
#include <QMovie>

#include <exiv2/exiv2.hpp>

#include <QDebug>

//TODO GENERAL: handle plurals in counts
//TODO GENERAL: create custom error boxes

Caesium::Caesium(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Caesium)
{
    ui->setupUi(this);

    initializeConnections();
    initializeUI();
    readPreferences();
    createMenuActions();
    createMenus();
    checkUpdates();

    QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());


}

Caesium::~Caesium() {
    delete ui;
}

void Caesium::initializeUI() {
    QSettings settings;

    //Install event filter for buttons
    ui->addFilesButton->installEventFilter(this);
    ui->addFolderButton->installEventFilter(this);
    ui->compressButton->installEventFilter(this);
    ui->removeItemButton->installEventFilter(this);
    ui->clearButton->installEventFilter(this);
    ui->settingsButton->installEventFilter(this);

    //Set the headers size
    ui->listTreeWidget->header()->resizeSection(0, 180);
    ui->listTreeWidget->header()->resizeSection(1, 100);
    ui->listTreeWidget->header()->resizeSection(2, 100);
    ui->listTreeWidget->header()->resizeSection(3, 80);
    ui->listTreeWidget->header()->resizeSection(4, 100);

    //Set side panel stretch
    ui->splitter->setStretchFactor(0, 2);

    //Set menu invisible for Windows/Linux
    //ui->menuBar->setVisible(false);

    //Restore window state
    settings.beginGroup(KEY_PREF_GROUP_GEOMETRY);
    resize(settings.value(KEY_PREF_GEOMETRY_SIZE, QSize(880, 500)).toSize());
    move(settings.value(KEY_PREF_GEOMETRY_POS, QPoint(200, 200)).toPoint());
    on_sidePanelDockWidget_visibilityChanged(settings.value(KEY_PREF_GEOMETRY_PANEL_VISIBLE).value<bool>());
    ui->listTreeWidget->sortByColumn(settings.value(KEY_PREF_GEOMETRY_SORT_COLUMN).value<int>(),
                                     settings.value(KEY_PREF_GEOMETRY_SORT_ORDER).value<Qt::SortOrder>());
    settings.endGroup();

    //No blue border on focus for Mac
    ui->listTreeWidget->setAttribute(Qt::WA_MacShowFocusRect, false);

    //Status bar widgets
    //Vertical lines
    statusStatusBarLine->setFrameShape(QFrame::VLine);
    statusStatusBarLine->setFrameShadow(QFrame::Raised);
    updateStatusBarLine->setFrameShape(QFrame::VLine);
    updateStatusBarLine->setFrameShadow(QFrame::Raised);
    updateStatusBarLine->setVisible(false);
    //List info label
    statusBarLabel->setText(" v" + versionString);
    //Update Button
    updateButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    updateButton->setAutoRaise(false);
    updateButton->setText(tr("A new version is available!"));
    updateButton->setIcon(QIcon(":/icons/ui/update.png"));
    updateButton->setVisible(false);
    //Add them to the status bar
    ui->statusBar->addPermanentWidget(statusStatusBarLine);
    ui->statusBar->addPermanentWidget(statusBarLabel);
    ui->statusBar->addPermanentWidget(updateStatusBarLine);
    ui->statusBar->addPermanentWidget(updateButton);

}

void Caesium::initializeConnections() {
    //Edit menu
    //List clear
    connect(ui->actionClear_list, SIGNAL(triggered()), ui->listTreeWidget, SLOT(clear()));
    connect(ui->actionClear_list, SIGNAL(triggered()), this, SLOT(updateStatusBarCount()));
    connect(ui->actionClear_list, SIGNAL(triggered()), this, SLOT(clearUI()));
    //List select all
    connect(ui->actionSelect_all, SIGNAL(triggered()), ui->listTreeWidget, SLOT(selectAll()));
    //UI buttons
    connect(ui->compressButton, SIGNAL(released()), this, SLOT(on_actionCompress_triggered()));
    connect(ui->addFilesButton, SIGNAL(released()), this, SLOT(on_actionAdd_pictures_triggered()));
    connect(ui->addFolderButton, SIGNAL(released()), this, SLOT(on_actionAdd_folder_triggered()));
    connect(ui->removeItemButton, SIGNAL(released()), this, SLOT(on_actionRemove_items_triggered()));
    connect(ui->clearButton, SIGNAL(released()), ui->listTreeWidget, SLOT(clear()));
    connect(ui->clearButton, SIGNAL(released()), this, SLOT(updateStatusBarCount()));
    connect(ui->clearButton, SIGNAL(released()), this, SLOT(clearUI()));

    //TreeWidget
    //Drop event
    connect(ui->listTreeWidget, SIGNAL(dropFinished(QStringList)), this, SLOT(showImportProgressDialog(QStringList)));
    //Context menu
    connect(ui->listTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showListContextMenu(QPoint)));

    //Update button
    connect(updateButton, SIGNAL(released()), this, SLOT(on_updateButton_clicked()));

    //List changed signal
    connect(ui->listTreeWidget, SIGNAL(itemsChanged()), this, SLOT(listChanged()));
}

void Caesium::readPreferences() {
    //Read important parameters from settings
    QSettings settings;

    settings.beginGroup(KEY_PREF_GROUP_GENERAL);
    params.overwrite = settings.value(KEY_PREF_GENERAL_OVERWRITE).value<bool>();
    params.outMethodIndex = settings.value(KEY_PREF_GENERAL_OUTPUT_METHOD).value<int>();
    params.outMethodString = settings.value(KEY_PREF_GENERAL_OUTPUT_STRING).value<QString>();
    settings.endGroup();
}

//Button hover functions
bool Caesium::eventFilter(QObject *obj, QEvent *event) {
    if (obj == (QObject*) ui->addFilesButton) {
        if (event->type() == QEvent::Enter && ui->addFilesButton->isEnabled()) {
            ui->addFilesButton->setIcon(QIcon(":/icons/ui/add_hover.png"));
            return true;
        } else if (event->type() == QEvent::Leave){
            ui->addFilesButton->setIcon(QIcon(":/icons/ui/add.png"));
            return true;
        } else {
            return false;
        }
    } else if (obj == (QObject*) ui->addFolderButton) {
        if (event->type() == QEvent::Enter && ui->addFolderButton->isEnabled()) {
            ui->addFolderButton->setIcon(QIcon(":/icons/ui/folder_hover.png"));
            return true;
        } else if (event->type() == QEvent::Leave){
            ui->addFolderButton->setIcon(QIcon(":/icons/ui/folder.png"));
            return true;
        } else {
            return false;
        }
    } else if (obj == (QObject*) ui->compressButton) {
        if (event->type() == QEvent::Enter && ui->compressButton->isEnabled()) {
            ui->compressButton->setIcon(QIcon(":/icons/ui/compress_hover.png"));
            return true;
        } else if (event->type() == QEvent::Leave){
            ui->compressButton->setIcon(QIcon(":/icons/ui/compress.png"));
            return true;
        } else {
            return false;
        }
    } else if (obj == (QObject*) ui->removeItemButton) {
        if (event->type() == QEvent::Enter && ui->removeItemButton->isEnabled()) {
            ui->removeItemButton->setIcon(QIcon(":/icons/ui/remove_hover.png"));
            return true;
        } else if (event->type() == QEvent::Leave){
            ui->removeItemButton->setIcon(QIcon(":/icons/ui/remove.png"));
            return true;
        } else {
            return false;
        }
    } else if (obj == (QObject*) ui->clearButton) {
        if (event->type() == QEvent::Enter && ui->clearButton->isEnabled()) {
            ui->clearButton->setIcon(QIcon(":/icons/ui/clear_hover.png"));
            return true;
        } else if (event->type() == QEvent::Leave){
            ui->clearButton->setIcon(QIcon(":/icons/ui/clear.png"));
            return true;
        } else {
            return false;
        }
    } else if (obj == (QObject*) ui->settingsButton) {
        if (event->type() == QEvent::Enter && ui->settingsButton->isEnabled()) {
            ui->settingsButton->setIcon(QIcon(":/icons/ui/settings_hover.png"));
            return true;
        } else if (event->type() == QEvent::Leave){
            ui->settingsButton->setIcon(QIcon(":/icons/ui/settings.png"));
            return true;
        } else {
            return false;
        }
    } else {
        //Pass the event on to the parent class
        return QWidget::eventFilter(obj, event);
    }
}

void Caesium::on_actionAbout_Caesium_triggered() {
    //Start the about dialog
    AboutDialog* ad = new AboutDialog(this);
    ad->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    ad->show();
}

void Caesium::on_actionAdd_pictures_triggered() {
    //Generate file dialog for import and call the progress dialog indicator
    QStringList fileList = QFileDialog::getOpenFileNames(this,
                                                         tr("Import files..."),
                                                         QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0),
                                                         inputFilter);
    if (!fileList.isEmpty()) {
        showImportProgressDialog(fileList);
    }
}

void Caesium::showImportProgressDialog(QStringList list) {
    QSettings settings;
    bool scanSubdir = settings.value(KEY_PREF_GROUP_GENERAL + KEY_PREF_GENERAL_SUBFOLDER).value<bool>();

    QProgressDialog progress(tr("Importing..."), tr("Cancel"), 0, list.count(), this);
    progress.setWindowIcon(QIcon(":/icons/main/logo.png"));
    progress.setWindowFlags(Qt::FramelessWindowHint);
    progress.setWindowModality(Qt::WindowModal);
    progress.show();

    //Actual added item count and duplicate count
    int item_count = 0;
    int duplicate_count = 0;

    for (int i = 0; i < list.size(); i++) {

        //Check if it's a folder
        if (QDir(list[i]).exists()) {
            //If so, add the whole content to the end of the list
            QDirIterator it(list[i], inputFilterList, QDir::AllEntries, scanSubdir ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags);
            while(it.hasNext()) {
                it.next();
                list.append(it.filePath());
            }
        }

        progress.setValue(i);        

        //Validate extension
        image_type img_type = detect_image_type(QStringToChar(list.at(i)));

        if (img_type == UNKN) {
            continue;
        }

        CTreeWidgetItem* item = new CTreeWidgetItem(ui->listTreeWidget, list.at(i));

        //Check if it has a duplicate
        if (hasADuplicateInList(item->image)) {
            duplicate_count++;
            continue;
        }

        //Populate list
        item->setText(COLUMN_NAME, item->image->getBaseName());
        item->setText(COLUMN_ORIGINAL_SIZE, item->image->getFormattedSize());
        item->setText(COLUMN_ORIGINAL_RESOLUTION, item->image->getFormattedResolution());
        item->setText(COLUMN_OPTIONS, tr("Default"));
        //item->setText(COLUMN_OPTIONS, item->image->getType() == PNG ? item->image->printPNGParams() : item->image->printJPEGParams());
        item->setText(COLUMN_PATH, item->image->getFullPath());

        ui->listTreeWidget->addTopLevelItem(item);

        item_count++;

        if (progress.wasCanceled()) {
            break;
        }
    }
    progress.setValue(list.count());

    //Show import stats in the status bar
    ui->statusBar->showMessage(duplicate_count > 0 ?
                                   QString::number(item_count) + tr(" files added to the list") + ", " +
                                   QString::number(duplicate_count) + tr(" duplicates found")
                                 : QString::number(item_count) + tr(" files added to the list"));
    updateStatusBarCount();
}

void Caesium::on_actionAdd_folder_triggered() {
    QString path = QFileDialog::getExistingDirectory(this,
                                                     tr("Select a folder to import..."),
                                                     QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0),
                                                     QFileDialog::ShowDirsOnly);
    if (!path.isEmpty()) {
        showImportProgressDialog(QStringList() << path);
    }
}

void Caesium::on_actionRemove_items_triggered() {
    int count = ui->listTreeWidget->selectedItems().count();
    if (count == ui->listTreeWidget->topLevelItemCount()) {
        ui->listTreeWidget->clear();
    } else {
        for (int i = 0; i < count; i++) {
            ui->listTreeWidget->takeTopLevelItem(ui->listTreeWidget->indexOfTopLevelItem(ui->listTreeWidget->selectedItems().at(0)));
        }
    }
    //Clear boxes
    clearUI();
    //Update count
    updateStatusBarCount();
    //Show a message
    ui->statusBar->showMessage(QString::number(count) + tr(" items removed"));
}

void Caesium::compressRoutine(CTreeWidgetItem* item, bool preview) {
    //TODO
    if (params.overwrite) {
        if (QMessageBox::warning(this,
                                 tr("Warning"),
                                 tr("All files will be overwritten. This cannot be undone.\n Continue?"),
                                 QMessageBox::Ok,
                                 QMessageBox::Cancel) == QMessageBox::Cancel) {
            return;
        }
    }
    //Input file path
    QString inputPath = item->text(COLUMN_PATH);
    QFileInfo* originalInfo = new QFileInfo(item->text(COLUMN_PATH));
    qint64 originalSize = originalInfo->size();
    QString outputPath;
    if (preview) {
        outputPath = previewPath;
    } else {
        outputPath = Caesium::getOutputPath(originalInfo);
    }

    if (!outputPath.isNull()) {
        qDebug() << item->text(COLUMN_PATH) << "into" << outputPath << " -- START";

        char* input = QStringToChar(inputPath);
        char* output = QStringToChar(outputPath);

        //Read parameters
        setParameters(item);

        //Not really necessary if we copy the whole EXIF data
        Exiv2::ExifData exifData = getExifFromPath(input);

        if (item->image->getType() == JPEG) {
            char* exif_orig = (char*) malloc(strlen(input) * sizeof(char));
            //Lossy processing just uses the compression method before optimizing
            if (item->image->jparams.getQuality() > 0) {
                cclt_jpeg_compress(output, cclt_jpeg_decompress(input, item->image), item->image);
                //TODO Check memory leaks
                //If we are using lossy compression, the input file is the output of
                //the previous function
                strcpy(exif_orig, input);
                input = output;
            }
            //Optimize
            cclt_jpeg_optimize(input, output, item->image, exif_orig);

            //Write important metadata as user requested
            if (item->image->jparams.getExif() && !item->image->jparams.getImportantExifs().isEmpty()) {
                writeSpecificExifTags(exifData, outputPath, item->image->jparams.getImportantExifs());
            }
        } else if (item->image->getType() == PNG) {
            cclt_png_optimize(inputPath, outputPath, &item->image->pparams);
        }

        //Gets new file info
        QFileInfo* fileInfo = new QFileInfo(outputPath);
        //Get the new size
        qint64 outputSize = fileInfo->size();

        //Check if the output file is actually bigger than the original
        if (outputSize > originalSize) {
            /*
             * If we choose to overwrite the files, just leave the files in the temporary folder
             * to be removed afterwards
             * Instead, if we compressed in a custom folder, copy the original over the compressed one
             * and set all the output results to point to the original file
             */
            qInfo() << "Output is bigger than input";
            if (!params.overwrite) {
                //Copy the original file over the compressed one
                QFile* outputFile = new QFile(outputPath);
                //Check if the file already exists (just a security check) and remove it
                if (outputFile->exists()) {
                    //WARNING No error check
                    outputFile->remove();
                }
                //Rename the original file with the output path
                //TODO Better error handling please
                if (!QFile(item->text(COLUMN_PATH)).copy(outputPath)) {
                    qCritical() << "Failed while moving " << item->text(COLUMN_PATH);
                }
            }
            //Set the importat stats to point to the original file
            outputSize = originalSize;
        } else {
            //The new file is smaller
            //If overwrite is on, move the file from the temp folder into the original
            if (params.overwrite) {
                //Remove the original
                QFile(item->text(COLUMN_PATH)).remove();
                //Move the compressed
                //TODO Better error handling please
                if (!QFile(outputPath).rename(item->text(COLUMN_PATH))) {
                    qCritical() << "Failed while moving " << item->text(COLUMN_PATH);
                }
            }
        }
        item->setText(COLUMN_NEW_SIZE, toHumanSize(outputSize));
        item->setText(COLUMN_SAVED, getRatio(originalSize, outputSize));

        //Global compression counters for the entire compression process
        originalsSize += originalSize;
        compressedSize += outputSize;
        compressedFiles++;
    }
}

QString Caesium::getOutputPath(QFileInfo* originalInfo) {
    QString outputPath;
    if (params.overwrite) {
        /*
         * Overwrite
         * Set the output path to a temporary directory
         * so we can check if it is actually bigger than the original
         * and eventually overwrite it
         *
        */
        if (tempDir.isValid()) {
            //Unique temporary directory
            outputPath = tempDir.path() + "/" + originalInfo->fileName();
        } else {
            qFatal("Cannot create a temporary folder. Abort.");
            exit(-1);
        }
    } else {
        QDir dir(originalInfo->path() + "/" + params.outMethodString + "/");
        switch (params.outMethodIndex) {
        case 0:
            //Add a suffix
            outputPath = originalInfo->filePath().replace(originalInfo->completeBaseName(),
                                                          originalInfo->baseName() + params.outMethodString);
            break;
        case 1:
            //Compress in a subfolder
            outputPath = originalInfo->path() + "/" + params.outMethodString + "/" + originalInfo->fileName();
            //Create it
            if (!dir.mkdir(dir.path()) && !dir.exists()) {
                ui->statusBar->showMessage(tr("ERROR: could not create output folder. Check user permissions."));
                qCritical() << "Cannot create output directory. Abort current operation";
                return NULL;
            }
            break;
        case 2:
            //Compress in a custom directory
            outputPath = params.outMethodString + "/" + originalInfo->fileName();
            if (!QDir().mkdir(params.outMethodString) && !QDir(params.outMethodString).exists()) {
                ui->statusBar->showMessage(tr("ERROR: could not create output folder. Check user permissions."));
                qCritical() << "Cannot create output directory. Abort current operation";
                return NULL;
            }
            break;
        default:
            break;
        }
    }

    return outputPath;
}

void Caesium::on_actionCompress_triggered() {
    //Return if list is empty
    if (ui->listTreeWidget->topLevelItemCount() < 1) {
        return;
    }
    //Read preferences again
    readPreferences();
    //Reset counters
    originalsSize = compressedSize = compressedFiles = 0;
    //Register metatype for emitting changes
    qRegisterMetaType<QVector<int> >("QVector<int>");

    //Setting up a progress dialog
    QProgressDialog progressDialog;
    progressDialog.setWindowTitle(tr("Caesium"));
    progressDialog.setLabelText(tr("Compressing..."));
    progressDialog.setWindowFlags(Qt::FramelessWindowHint);

    //Holds the list
    QList<CTreeWidgetItem*> list;

    //Setup watcher
    QFutureWatcher<void> watcher;

    //Gets the list filled
    for (int i = 0; i < ui->listTreeWidget->topLevelItemCount(); i++) {
        list.append((CTreeWidgetItem*) ui->listTreeWidget->topLevelItem(i));
    }

    QFuture<void> future = QtConcurrent::map(list, [this] (CTreeWidgetItem*& data) {compressRoutine(data);});

    //Setting up connections
    //Progress dialog
    connect(&watcher, SIGNAL(progressValueChanged(int)), &progressDialog, SLOT(setValue(int)));
    connect(&watcher, SIGNAL(progressRangeChanged(int, int)), &progressDialog, SLOT(setRange(int,int)));
    connect(&watcher, SIGNAL(finished()), &progressDialog, SLOT(reset()));
    connect(&progressDialog, SIGNAL(canceled()), &watcher, SLOT(cancel()));
    //Connect two slots for handling compression start/finish
    connect(&watcher, SIGNAL(started()), this, SLOT(compressionStarted()));
    connect(&watcher, SIGNAL(finished()), this, SLOT(compressionFinished()));

    //And start
    watcher.setFuture(future);

    //Show the dialog
    progressDialog.exec();
}

void Caesium::compressionStarted() {
    //TODO Initialize parameters
    //Start monitoring time while compressing
    timer.start();
}

void Caesium::compressionFinished() {
    //Get elapsed time of the compression
    qInfo() << "Finished compression at " << QTime::currentTime();

    //Display statistics in the status bar
    ui->statusBar->showMessage(tr("Compression completed! ") +
                               QString::number(compressedFiles) + tr(" files compressed in ") +
                               msToFormattedString(timer.elapsed()) + ", " +
                               tr("from ") + toHumanSize(originalsSize) + tr(" to ") + toHumanSize(compressedSize) +
                               ". " + tr("Saved ") + toHumanSize(originalsSize - compressedSize) +
                               " (" + getRatio(originalsSize, compressedSize) + ")"
                               );
    timer.invalidate();
}

void Caesium::on_sidePanelDockWidget_topLevelChanged(bool topLevel) {

}

void Caesium::on_sidePanelDockWidget_visibilityChanged(bool visible) {
    //Handle the close event
    on_showSidePanelButton_clicked(visible);
}

void Caesium::on_showSidePanelButton_clicked(bool checked) {

}


void Caesium::on_listTreeWidget_itemSelectionChanged() {
    bool itemsSelected = ui->listTreeWidget->selectedItems().length() > 0;
    //Check if there's a selection
    if (itemsSelected) {
        ui->imageCompressedPreviewLabel->clear();
        //Get the first item selected
        CTreeWidgetItem* currentItem = (CTreeWidgetItem*) ui->listTreeWidget->selectedItems().at(0);

        //Try to load a preview

        previewPath = calculatePreviewHashPath(currentItem);
        qDebug() << "Preview path is " << previewPath;

        if (QFile(previewPath).exists()) {
            qDebug() << "Preview exists at " << previewPath;
            connect(&imagePreviewWatcher, SIGNAL(resultReadyAt(int)), this, SLOT(finishPreviewLoading(int)));
            imagePreviewWatcher.setFuture(QtConcurrent::run<QImage>(this, &Caesium::loadImage, previewPath));
        }

        //Connect the global watcher to the slot
        connect(&imageWatcher, SIGNAL(resultReadyAt(int)), this, SLOT(finishImageLoading(int)));

        //Run the image loader function
        imageWatcher.setFuture(QtConcurrent::run<QImage>(this, &Caesium::loadImage, currentItem->text(COLUMN_PATH)));
    } else {
        imageWatcher.cancel();
        clearUI();
    }

    //Rule the remove button/action
    ui->actionRemove_items->setEnabled(itemsSelected);
    ui->removeItemButton->setEnabled(itemsSelected);
}

QImage Caesium::loadImage(QString path) {
    //Load a scaled version of the image into memory
    QImageReader* imageReader = new QImageReader(path);
    imageReader->setScaledSize(getScaledSizeWithRatio(imageReader->size(), ui->imagePreviewLabel->size().width()));
    return imageReader->read();
}

void Caesium::finishImageLoading(int i) {
    //Set the image
    ui->imagePreviewLabel->setPixmap(QPixmap::fromImage(imageWatcher.resultAt(i)));
}

void Caesium::finishPreviewLoading(int i) {
    //Set the image
    qDebug() << "PREVIEW FINISHED!";
    ui->imageCompressedPreviewLabel->setPixmap(QPixmap::fromImage(imagePreviewWatcher.resultAt(i)));
}

void Caesium::on_settingsButton_clicked() {
    PreferenceDialog* pd = new PreferenceDialog(this);
    pd->show();
}

void Caesium::closeEvent(QCloseEvent *event) {
    QSettings settings;

    //Save window geometry
    settings.beginGroup(KEY_PREF_GROUP_GEOMETRY);
    settings.setValue(KEY_PREF_GEOMETRY_SIZE, size());
    settings.setValue(KEY_PREF_GEOMETRY_POS, pos());
    settings.setValue(KEY_PREF_GEOMETRY_SORT_COLUMN, ui->listTreeWidget->sortColumn());
    settings.setValue(KEY_PREF_GEOMETRY_SORT_ORDER, ui->listTreeWidget->header()->sortIndicatorOrder());
    settings.endGroup();

    if (settings.value(KEY_PREF_GROUP_GENERAL + KEY_PREF_GENERAL_PROMPT).value<bool>()) {
        //Display a prompt
        int res = QMessageBox::warning(this, tr("Caesium"),
                                       tr("Do you really want to exit?"),
                                       QMessageBox::Ok | QMessageBox::Cancel);
        //Exit if OK, go back if Cancel
        switch (res) {
        case QMessageBox::Ok:
            qInfo() << "----------------- Caesium session stopped at "
                    << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << "-----------------";
            QFile(logPath).close();
            event->accept();
            break;
        case QMessageBox::Cancel:
            event->ignore();
        default:
            break;
        }
    } else {
        qInfo() << "----------------- Caesium session stopped at "
                << QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss") << "-----------------";
        QFile(logPath).close();
        event->accept();
    }
}

void Caesium::checkUpdates() {
    qInfo() << "Check for updates called";
    NetworkOperations* op = new NetworkOperations();
    op->checkForUpdates();
    connect(op, SIGNAL(updateDownloadFinished(QString)), this, SLOT(updateDownloadFinished(QString)));
    connect(op, SIGNAL(checkForUpdatesFinished(int, QString, QString)), this, SLOT(updateAvailable(int, QString, QString)));
}

void Caesium::updateAvailable(int version, QString versionTag, QString checksum) {
    qInfo() << "Connection to update server was succesful. Latest build is " << version;
    updateVersionTag = versionTag;
    if (version > versionNumber) {
        NetworkOperations* op = new NetworkOperations();
        connect(op, SIGNAL(updateDownloadFinished(QString)), this, SLOT(updateDownloadFinished(QString)));
        op->downloadUpdateRequest(checksum);
    } else {
        qInfo() << "Your version is equal or higher than the remote";
    }
}

bool Caesium::hasADuplicateInList(CImage *c) {
    for (int i = 0; i < ui->listTreeWidget->topLevelItemCount(); i++) {
        if (c->isEqual(ui->listTreeWidget->topLevelItem(i)->text(COLUMN_PATH))) {
            qInfo() << "Duplicate detected. Skipping";
            return true;
        }
    }
    return false;
}

void Caesium::on_updateButton_clicked() {
    //Show a confirmation dialog
    int ret = QMessageBox::warning(this,
                                   tr("Update available"),
                                   tr("This will close Caesium. Do you really want to update now?"),
                                   QMessageBox::Ok | QMessageBox::Cancel);
    if (ret == QMessageBox::Ok) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(updatePath));
        this->close();
    }
}

void Caesium::updateDownloadFinished(QString path) {
    updateButton->setVisible(true);
    updateStatusBarLine->setVisible(true);
    updatePath = path;
}

void Caesium::clearUI() {
    ui->imagePreviewLabel->setText(tr("original"));
    ui->imageCompressedPreviewLabel->setText(tr("compressed"));
}

void Caesium::updateStatusBarCount() {
    statusBarLabel->setText(
                QString::number(ui->listTreeWidget->topLevelItemCount()) +
                tr(" files in list"));

    //If the list is empty, we got a call from the clear SIGNAL, so handle the general message too
    if (ui->listTreeWidget->topLevelItemCount() == 0) {
        ui->statusBar->showMessage(tr("List cleared"));
    }

    //Emit the itemsChanged SIGNAL for the TreeWidget
    emit ui->listTreeWidget->itemsChanged();
}

void Caesium::on_actionShow_input_folder_triggered() {
    //Open the input folder
    QDesktopServices::openUrl(QUrl("file:///" +
                                   QFileInfo(ui->listTreeWidget->selectedItems().at(0)->text(COLUMN_PATH)).dir().absolutePath(),
                                   QUrl::TolerantMode));
}

void Caesium::on_actionShow_output_folder_triggered() {
    //Read preferences first
    readPreferences();
    //Open the output folder
    QDesktopServices::openUrl(QUrl("file:///" +
                                   QFileInfo(
                                       Caesium::getOutputPath(
                                           new QFileInfo(ui->listTreeWidget->selectedItems().at(0)->text(COLUMN_PATH)))).dir().absolutePath(),
                                   QUrl::TolerantMode));
}

void Caesium::createMenuActions() {
    //List Remove action
    listRemoveAction = new QAction(tr("Remove item"), this);
    listRemoveAction->setStatusTip(tr("Remove the item from the list"));
    connect(listRemoveAction, SIGNAL(triggered()), this, SLOT(on_actionRemove_items_triggered()));

    //List source dir action
    listShowInputFolderAction = new QAction(tr("Show in folder"), this);
    listShowInputFolderAction->setStatusTip(tr("Opens the folder containing the file"));
    connect(listShowInputFolderAction, SIGNAL(triggered()), this, SLOT(on_actionShow_input_folder_triggered()));

    //List output dir action
    listShowOutputFolderAction = new QAction(tr("Show destination folder"), this);
    listShowOutputFolderAction->setStatusTip(tr("Opens the destination folder for the file"));
    connect(listShowOutputFolderAction, SIGNAL(triggered()), this, SLOT(on_actionShow_output_folder_triggered()));

    //List clear action
    listClearAction = new QAction(tr("Clear list"), this);
    listClearAction->setStatusTip(tr("Clears the list"));
    connect(listClearAction, SIGNAL(triggered()), ui->listTreeWidget, SLOT(clear()));
    connect(listClearAction, SIGNAL(triggered()), this, SLOT(clearUI()));
    connect(listClearAction, SIGNAL(triggered()), this, SLOT(updateStatusBarCount()));
}

void Caesium::createMenus() {
    //Creates the list context menu
    listMenu = new QMenu(this);
    listMenu->addAction(listRemoveAction);
    listMenu->addSeparator();
    listMenu->addAction(listShowInputFolderAction);
    listMenu->addAction(listShowOutputFolderAction);
    listMenu->addSeparator();
    listMenu->addAction(listClearAction);
}

void Caesium::showListContextMenu(QPoint pos) {
    //No menu if the there're no items int he list
    if (ui->listTreeWidget->topLevelItemCount() > 0) {
        //Check if we have the same root folder in the selection
        //and activate the IN menu option
        listShowInputFolderAction->setEnabled(haveSameRootFolder(ui->listTreeWidget->selectedItems()));
        listMenu->exec(ui->listTreeWidget->mapToGlobal(pos));
    }
}

void Caesium::on_actionSave_list_triggered() {
    //If the path is not set, we need to call the saveAs instead
    if (lastCListPath.isEmpty()) {
        on_actionSave_list_as_triggered();
    } else {
        //Save to the last path
        saveCListToFile(lastCListPath);
        //And disable yourself
        ui->actionSave_list->setEnabled(false);
    }
}

void Caesium::on_actionSave_list_as_triggered() {
    //Give a path if it's not passed
    QString path = QFileDialog::getSaveFileName(this,
                                                tr("Save list as..."),
                                                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                clfFilter);
    //Call the generic function
    saveCListToFile(path);
}

void Caesium::saveCListToFile(QString path) {
    QList<QTreeWidgetItem* > list;
    //Check if it's valid
    if (!path.isEmpty()) {
        //Get a list of items
        for (int i = 0; i < ui->listTreeWidget->topLevelItemCount(); i++) {
            list.append(ui->listTreeWidget->topLevelItem(i));
        }
        //And generate the file
        CList* clf = new CList();
        clf->writeToFile(list, path);
        //Set the global path
        lastCListPath = path;
        //Deactivate the "save" action
        ui->actionSave_list->setEnabled(false);
    }
}

void Caesium::setParameters(CTreeWidgetItem *item)
{
    if (item->image->getType() == JPEG) {
        if (ui->losslessCheckBox->isChecked()) {
            item->image->jparams.setQuality(0);
        } else {
            item->image->jparams.setQuality(ui->qualitySlider->value());
        }
        item->image->jparams.setExif(ui->exifCheckBox->isChecked());
        item->image->jparams.setProgressive(ui->progressiveCheckBox->isChecked());
        QList<cexifs> exifs;
        if (ui->copyrightCheckBox->isChecked()) {
            exifs.append(EXIF_COPYRIGHT);
        }
        if (ui->dateCheckBox->isChecked()) {
            exifs.append(EXIF_DATE);
        }
        if (ui->commentsCheckBox->isChecked()) {
            exifs.append(EXIF_COMMENTS);
        }
        item->image->jparams.setImportantExifs(exifs);
    } else if (item->image->getType() == PNG) {
        item->image->pparams.setIterations(ui->iterationsSpinBox->value());
        item->image->pparams.setIterationsLarge(ui->iterationsLargeSpinBox->value());
        item->image->pparams.setLossy8Bit(ui->lossy8CheckBox->isChecked());
    }
}

void Caesium::on_actionOpen_list_triggered() {
    //TODO Run an integrity check for the imported data and edit eventually
    //Get the filepath
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    tr("Import files..."),
                                                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                    clfFilter);
    //If it's valid and not empty
    if (!filePath.isEmpty()) {
        //Clear the list first
        ui->listTreeWidget->clear();
        //Create an instance of the reader
        CList* clf = new CList();
        //Read the file
        ui->listTreeWidget->addTopLevelItems(clf->readFile(filePath));
        //Set the global path
        lastCListPath = filePath;
        //Deactivate the "save" action
        ui->actionSave_list->setEnabled(false);
        //But be sure the saveAs is enabled
        ui->actionSave_list_as->setEnabled(true);
        //Update UI count
        updateStatusBarCount();
    }
}

void Caesium::listChanged() {
    //List changed, so we need to enable the save feature
    ui->actionSave_list->setEnabled(true);
    ui->actionSave_list_as->setEnabled(true);

    //If the list is empty, we don't need the clear button
    ui->actionClear_list->setDisabled(ui->listTreeWidget->topLevelItemCount() == 0);
    ui->clearButton->setDisabled(ui->listTreeWidget->topLevelItemCount() == 0);

    //Background image for the list
    if (ui->listTreeWidget->topLevelItemCount() != 0) {
        ui->listTreeWidget->setStyleSheet("QTreeWidget#listTreeWidget {background: #ffffff;}");
    } else {
        ui->listTreeWidget->setStyleSheet("QTreeWidget#listTreeWidget {background: url(:/icons/main/logo_alpha.png) no-repeat center;}");
    }
}

void Caesium::testSignal() {
    qDebug() << "TEST SLOT TRIGGERED";
}

void Caesium::on_exifTextEdit_textChanged() {

}

void Caesium::startPreviewLoading() {
    QMovie* loader = new QMovie(":/icons/ui/loader.gif");
    ui->imagePreviewLabel->setMovie(loader);
    loader->start();
}


void Caesium::on_applyButton_clicked() {
    if (ui->listTreeWidget->selectedItems().length() > 0) {
        foreach (QTreeWidgetItem* qItem, ui->listTreeWidget->selectedItems()) {
            CTreeWidgetItem* item = (CTreeWidgetItem*) qItem;
            setParameters(item);
            if (item->image->getType() == JPEG) {
                item->setText(COLUMN_OPTIONS, item->image->printJPEGParams());
            } else if (item->image->getType() == PNG) {
                item->setText(COLUMN_OPTIONS, item->image->printPNGParams());
            }
        }
    }
}

void Caesium::on_previewButton_clicked() {
    if (ui->listTreeWidget->selectedItems().length() > 0) {
        //Get the first item selected
        CTreeWidgetItem* currentItem = (CTreeWidgetItem*) ui->listTreeWidget->selectedItems().at(0);

        //TODO Preview for all?
        previewList.append(currentItem);
        QFuture<void> future = QtConcurrent::map(previewList, [this] (CTreeWidgetItem*& data) {compressRoutine(data, true);});

        watcher = new QFutureWatcher<void>(this);
        connect(watcher, SIGNAL(finished()), this, SLOT(loadPreview()));
        previewPath = calculatePreviewHashPath(currentItem);
        if (!QFile(previewPath).exists()) {
            qRegisterMetaType<QVector<int> >("QVector<int>");

            //And start
            watcher->setFuture(future);
        }
    } else {
        imagePreviewWatcher.cancel();
        clearUI();
    }
}

void Caesium::loadPreview() {
    previewList.clear();
    connect(&imagePreviewWatcher, SIGNAL(resultReadyAt(int)), this, SLOT(finishPreviewLoading(int)));
    imagePreviewWatcher.setFuture(QtConcurrent::run<QImage>(this, &Caesium::loadImage, previewPath));
}

QString Caesium::calculatePreviewHashPath(CTreeWidgetItem *currentItem) {
    QByteArray hash;
    QString previewPath, toBeHashed;

    //We need to calculate if there's already a preview with same options
    if (currentItem->image->getType() == PNG) {
        //The string to be hashed is a combination of filename + options
        //TODO Better looking string?
        toBeHashed = currentItem->text(COLUMN_PATH) + currentItem->image->printPNGParams();
    } else if (currentItem->image->getType() == JPEG) {
        toBeHashed = currentItem->text(COLUMN_PATH) + currentItem->image->printJPEGParams();
    }
    qDebug() << "To be hashed: " << toBeHashed;
    hash = QCryptographicHash::hash(toBeHashed.toUtf8(), QCryptographicHash::Sha256);
    previewPath = tempDir.path() + "/" + QString(hash.toHex());

    return previewPath;
}
