#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "utils.h"

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    setupUI();
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

void AboutDialog::setupUI() {
    //Add logo
    QGraphicsScene* scene = new QGraphicsScene(0, 0, 100, 100, this);
    QGraphicsPixmapItem* gItem = new QGraphicsPixmapItem(QIcon(":/icons/main/logo.png").pixmap(96));
    scene->addItem(gItem);
    ui->logoGraphicsView->setScene(scene);

    //Version label
    ui->versionLabel->setText(tr("Version") +
                              " <strong>" + versionString +
                              "</strong><br /> (build " +
                              QString::number(buildNumber) + ")");
}
