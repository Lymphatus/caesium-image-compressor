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

#ifndef CAESIUM_H
#define CAESIUM_H

#include "cimage.h"
#include "clist.h"
#include "ctreewidgetitem.h"

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QFutureWatcher>
#include <QTime>
#include <QToolButton>
#include <QLabel>
#include <QFileInfo>

namespace Ui {
class Caesium;
}

class Caesium : public QMainWindow
{
    Q_OBJECT

public:
    explicit Caesium(QWidget *parent = 0);
    bool eventFilter(QObject *obj, QEvent *event);
    ~Caesium();
    static Caesium* instance() {
        static Caesium window;
        return &window;
    }

    //Gets the right output folder
    QString getOutputPath(QFileInfo *originalInfo);

    //Compress routine
    void compressRoutine(CTreeWidgetItem* );

signals:
    void dropAccepted(QStringList);

private slots:
    void on_actionAbout_Caesium_triggered();
    void on_actionAdd_pictures_triggered();
    void on_actionAdd_folder_triggered();
    void on_actionRemove_items_triggered();
    void on_actionCompress_triggered();
    void compressionStarted();
    void compressionFinished();
    void on_sidePanelDockWidget_topLevelChanged(bool topLevel);
    void on_sidePanelDockWidget_visibilityChanged(bool visible);
    void on_showSidePanelButton_clicked(bool checked);
    void on_listTreeWidget_itemSelectionChanged();
    QImage loadImagePreview(QString path);
    void finishPreviewLoading(int i);
    void closeEvent(QCloseEvent *event);
    void on_settingsButton_clicked();
    void showImportProgressDialog(QStringList);
    void updateAvailable(int, QString, QString);
    void on_updateButton_clicked();
    void updateDownloadFinished(QString);
    void clearUI();
    void updateStatusBarCount();
    void showListContextMenu(QPoint);
    void on_actionShow_input_folder_triggered();
    void on_actionShow_output_folder_triggered();
    void on_actionSave_list_triggered();
    void on_actionSave_list_as_triggered();
    void on_actionOpen_list_triggered();
    void listChanged();
    //TODO Remove, just test slot
    void testSignal();
    void on_exifTextEdit_textChanged();
    void startPreviewLoading();


    void on_applyButton_clicked();


private:
    Ui::Caesium *ui;
    QFutureWatcher<QImage> imageWatcher; //Image preview loader
    //Status bar widgets
    QToolButton* updateButton = new QToolButton();
    QFrame* statusStatusBarLine = new QFrame();
    QFrame* updateStatusBarLine = new QFrame();
    QLabel* statusBarLabel = new QLabel();
    QString updatePath;
    QString inputFilter = QIODevice::tr("Image Files") + " (*.jpg *.jpeg *.png)";

    //List Menu
    QMenu* listMenu;
    //List menu actions
    QAction* listRemoveAction;
    QAction* listShowInputFolderAction;
    QAction* listShowOutputFolderAction;
    QAction* listClearAction;


    void initializeConnections();
    void initializeUI();
    void readPreferences();

    //Update
    void checkUpdates();
    //Menus
    void createMenuActions();
    void createMenus();

    //Check list duplicates
    bool hasADuplicateInList(CImage* c);

    //CList save function
    void saveCListToFile(QString path);

};

#endif // CAESIUM_H
