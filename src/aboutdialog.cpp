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
