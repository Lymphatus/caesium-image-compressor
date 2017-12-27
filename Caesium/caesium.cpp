#include "caesium.h"
#include "ui_caesium.h"

#include <QGraphicsDropShadowEffect>

Caesium::Caesium(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Caesium)
{
    ui->setupUi(this);

    //Initialization
    initUI();
}

Caesium::~Caesium()
{
    delete ui;
}

void Caesium::initUI()
{
    QGraphicsDropShadowEffect *listDropShadowEffect = new QGraphicsDropShadowEffect();
    listDropShadowEffect->setBlurRadius(20);
    listDropShadowEffect->setXOffset(0);
    listDropShadowEffect->setYOffset(0);
    listDropShadowEffect->setColor(QColor(0, 0, 0, 50));
    ui->list_TreeWidget->setGraphicsEffect(listDropShadowEffect);

    QGraphicsDropShadowEffect *originalPreviewDropShadowEffect = new QGraphicsDropShadowEffect();
    originalPreviewDropShadowEffect->setBlurRadius(20);
    originalPreviewDropShadowEffect->setXOffset(0);
    originalPreviewDropShadowEffect->setYOffset(0);
    originalPreviewDropShadowEffect->setColor(QColor(0, 0, 0, 50));
    ui->originalImagePreview_Label->setGraphicsEffect(originalPreviewDropShadowEffect);

    QGraphicsDropShadowEffect *compressedPreviewDropShadowEffect = new QGraphicsDropShadowEffect();
    compressedPreviewDropShadowEffect->setBlurRadius(20);
    compressedPreviewDropShadowEffect->setXOffset(0);
    compressedPreviewDropShadowEffect->setYOffset(0);
    compressedPreviewDropShadowEffect->setColor(QColor(0, 0, 0, 50));
    ui->compressedImagePreview_Label->setGraphicsEffect(compressedPreviewDropShadowEffect);

    QGraphicsDropShadowEffect *optionsDropShadowEffect = new QGraphicsDropShadowEffect();
    optionsDropShadowEffect->setBlurRadius(20);
    optionsDropShadowEffect->setXOffset(0);
    optionsDropShadowEffect->setYOffset(0);
    optionsDropShadowEffect->setColor(QColor(0, 0, 0, 50));
    ui->options_Frame->setGraphicsEffect(optionsDropShadowEffect);
}

