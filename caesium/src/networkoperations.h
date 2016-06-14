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

#ifndef NETWORKOPERATIONS_H
#define NETWORKOPERATIONS_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressDialog>
#include <QFile>

class NetworkOperations : public QObject
{
    Q_OBJECT

public:
    explicit NetworkOperations(QObject *parent = 0);
    void checkForUpdates();
    void downloadUpdateRequest(QString checksum);


public slots:
    void getCurrentBuild();
    void flushUpdate(); 
    int compareUpdateChecksums(QString checksum, QByteArray *file);

signals:
    void checkForUpdatesFinished(int, QString, QString);
    void updateDownloadFinished(QString);

private:
    QNetworkAccessManager* networkManager = new QNetworkAccessManager();
    QNetworkReply* updateReply;
    QNetworkReply* downloadUpdateReply;
    QString releaseURL;
    QProgressDialog* pDialog;
    QString updateChecksum;
    QString updatePath;

};

#endif // NETWORKOPERATIONS_H
