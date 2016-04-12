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

#include "networkoperations.h"
#include "utils.h"


#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProgressDialog>
#include <QStandardPaths>

NetworkOperations::NetworkOperations(QObject *parent) : QObject(parent) {
    releaseURL = "https://github.com/Lymphatus/caesium-image-compressor/releases/download/v" +
            updateVersionTag + "/caesium-" + updateVersionTag +
            osAndExtension.at(1);

    updatePath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
            "/" +
            "caesium" +
            osAndExtension.at(1);
}

void NetworkOperations::checkForUpdates() {
    //Request current build from network
    //TODO Rewrite with GitHub API
    QNetworkRequest request;
    request.setUrl(QUrl("http://download.saerasoft.com/caesium/current"));
    request.setRawHeader( "User-Agent" , "Mozilla Firefox" );
    updateReply = networkManager->get(request);
    connect(updateReply, SIGNAL(readyRead()), this, SLOT(getCurrentBuild()));
}

void NetworkOperations::getCurrentBuild() {
    //Actually gets the build number
    if (updateReply->error() == QNetworkReply::NoError) {
        int v_number = updateReply->readLine().split('\n').at(0).toInt();
        QString v_string = updateReply->readLine().replace("\n", "");

        qInfo() << "Latest remote version is" << v_number << v_string;

        //Start reading for checksums
        QString line = updateReply->readLine();
        while (!line.isEmpty()) {
            //Read the right release checksum
            if (line.contains(osAndExtension.at(0))) {
                line = line.split(' ').at(1);
                updateChecksum = line.replace("\n", "");
                qInfo() << "Remote update checksum is " << updateChecksum;
                break;
            }
            line = updateReply->readLine();
        }
        //If the file already exists check it's the correct one
        QFile updateFile(updatePath);
        qInfo() << updatePath << "exists?" << updateFile.exists();
        if (updateFile.exists()) {
            qInfo() << "Already found an update";
            if (updateFile.open(QFile::ReadOnly)) {
                QByteArray data = updateFile.readAll();
                qInfo() << "Checking integrity";
                if (compareUpdateChecksums(updateChecksum, &data) != 0) {
                    qInfo() << "Different checksums";
                    emit checkForUpdatesFinished(v_number,
                                                 v_string,
                                                 updateChecksum);
                } else if (v_number > versionNumber){
                    qInfo() << "Checksums are equal, skip downloading";
                    emit updateDownloadFinished(updatePath);
                }
            } else {
                qCritical() << "Failed to open the already downloaded update";
            }
        } else {
            qInfo() << "No already downloaded update found. Get it";
            emit checkForUpdatesFinished(v_number,
                                         v_string,
                                         updateChecksum);
        }
    } else {
        qCritical() << "Failed to get latest release build. Error: " << updateReply->errorString();
    }
    updateReply->close();
}

void NetworkOperations::downloadUpdateRequest(QString checksum) {
    updateChecksum = checksum;
    //Set the right URL according to OS
    QUrl url;
    url.setUrl(releaseURL);

    //Build a request and set an header
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader( "User-Agent" , "Mozilla Firefox" );

    //Get request
    downloadUpdateReply = networkManager->get(request);
    qInfo() << "Update GET request started";

    //Connections
    connect(downloadUpdateReply, SIGNAL(finished()), this, SLOT(flushUpdate()));
}

void NetworkOperations::flushUpdate() {
    if (downloadUpdateReply->error() == QNetworkReply::NoError) {        
        //Check if we are being redirected
        QVariant variant = downloadUpdateReply->attribute(QNetworkRequest::RedirectionTargetAttribute);

        if (!variant.isNull()) {
            //Handle redirection; abort the current operation
            downloadUpdateReply->abort();
            //Change the url to the new one
            releaseURL = variant.toString();
            //Go again
            NetworkOperations::downloadUpdateRequest(updateChecksum);
            //Don't forget to return to abort the current function
            return;
        }

        QFile *file = new QFile(updatePath);

        //Flush the file
        if (file->open(QFile::WriteOnly)) {
            QByteArray data = downloadUpdateReply->readAll();
            file->write(data);
            file->flush();
            qInfo() << "Download finished and flushed";
            if (compareUpdateChecksums(updateChecksum, &data) == 0) {
                file->close();
                //Correct checksum
                qInfo() << "Correct checksum";
                emit updateDownloadFinished(updatePath);
            } else {
                //Failure
                //By now, retry on the next Caesium start
            }
            downloadUpdateReply->deleteLater();
        } else {
            downloadUpdateReply->deleteLater();
            qCritical() << "Cannot write update to file";
        }
    } else if (downloadUpdateReply->error() == QNetworkReply::OperationCanceledError) {
        qCritical() << "Update write aborted";
    }
}

int NetworkOperations::compareUpdateChecksums(QString checksum, QByteArray* file) {
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(*file);
    QString downloadedChecksum = hash.result().toHex();
    qInfo() << "Comparing checksums. Original: " << checksum << "\nDownloaded: " << downloadedChecksum;
    return QString::compare(downloadedChecksum, checksum);
}
