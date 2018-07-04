#ifndef CAESIUM_H
#define CAESIUM_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QStringList>
#include <QFutureWatcher>
#include <QList>
#include <QImage>
#include "src/ctreewidgetitem.h"

namespace Ui {
class Caesium;
}

class Caesium : public QMainWindow
{
    Q_OBJECT

public:
    explicit Caesium(QWidget *parent = 0);
    ~Caesium();

private slots:
    void closeEvent(QCloseEvent *event);
    void updateStatusBar();
    //Actions
    void on_actionAbout_Caesium_Image_Compressor_triggered();
    void on_actionAdd_files_triggered();
    void on_actionPreferences_triggered();
    void on_actionAdd_folder_triggered();
    void onImportStarted();
    void onImportFinished();

    //TODO Test only
    void testSlot(int value);

    void on_list_TreeWidget_itemSelectionChanged();
    void on_listChanged();
    QImage loadImage(QString path);
    void onOriginalImageLoadingFinished(int index);

    void on_actionClear_list_triggered();

    void on_actionRemove_triggered();

private:
    Ui::Caesium *ui;
    QString inputFilter = QIODevice::tr("Image Files") + " (*.jpg *.jpeg *.png)";
    QStringList inputFilterList = QStringList() << "*.jpg" << "*.jpeg" << "*.png";

    QFutureWatcher<QImage> *originalImageWatcher;

    void initMembers();
    void initConnections();

    void initUI();
    void toggleUI(bool enabled);
    void clearUI();

    QStringList scanFolder(QString path);
    void importFiles(QStringList filesList);
    void importFiles(QString folder);
    void populateListItems(CTreeWidgetItem* item);


};

#endif // CAESIUM_H
