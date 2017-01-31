#include "updater.h"

#include <QDebug>
#include <QNetworkReply>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDir>

//TODO Manage SSL errors

Updater::Updater(QObject *parent) : QObject(parent) {
    rootUrl = "http://download.saerasoft.com/caesium/" + osAndExtension.at(0) + "/";
    rootFolder = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
            "/updates";
    if (!QDir(rootFolder).exists()) {
        QDir().mkpath(rootFolder);
    }
    networkManager = new QNetworkAccessManager();
    qInfo() << "Root URL is" << rootUrl;
    qInfo() << "Root download folder is" << rootFolder;
}

void Updater::requestReleaseList() {
    QNetworkRequest fileListRequest;
    fileListRequest.setUrl(QUrl(rootUrl + "rules"));
    qInfo() << "Requesting" << rootUrl + "rules";
    fileListRequest.setRawHeader( "User-Agent" , "Mozilla Firefox" );
    fileListReply = networkManager->get(fileListRequest);
    connect(fileListReply, SIGNAL(finished()), this, SLOT(getReleaseFiles()));
}

void Updater::requestFileDownload(release_file file) {
    qInfo() << "Requesting" << rootUrl + file.path << "for download";
    QNetworkRequest request(QUrl(rootUrl + file.path));
    QNetworkReply *reply = networkManager->get(request);
    hash.insert(reply, file);

    currentDownloads.append(reply);
}

void Updater::getReleaseFiles() {
    if (fileListReply->error() == QNetworkReply::NoError && fileListReply->isReadable()) {
        qInfo() << "Rules OK. Parsing data...";
        QString line = fileListReply->readLine();
        while (!line.isEmpty()) {
            releaseList.append(parseReleaseFile(line.replace("\n", "").split("||")));
            qDebug() << line.split("||");
            line = fileListReply->readLine();
        }
    } else {
        qCritical() << "Could not read rules file. Error: " << fileListReply->errorString();
        exit(EXIT_FAILURE);
    }
    fileListReply->close();
    if (!releaseList.isEmpty()) {
        connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
        foreach (release_file file, releaseList) {
            requestFileDownload(file);
        }
    } else {
        qCritical() << "Empty file list.";
        exit(EXIT_FAILURE);
    }
}

bool Updater::saveToDisk(const QString &filename, QIODevice *data, release_file* r_file) {
    QFile file(filename);
    QByteArray responseContent = data->readAll();
    if (compareChecksums(r_file->hash, &responseContent) != 0) {
        qWarning() << "Incorrect checksums.";
        return false;
    }
    if (!file.open(QIODevice::WriteOnly)) {
        fprintf(stderr, "Could not open %s for writing: %s\n",
                qPrintable(filename),
                qPrintable(file.errorString()));
        return false;
    }

    file.write(responseContent);
    file.close();

    return true;
}

QString Updater::getDownloadPath(QUrl url) {
    QString path = url.path().replace("/caesium/" + osAndExtension.at(0) + "/", "");
    QString basename = QFileInfo(path).fileName();

    if (!QFileInfo(rootFolder + "/" + path).absoluteDir().exists()) {
        QDir().mkpath(QFileInfo(rootFolder + "/" + path).absolutePath());
    }

    if (basename.isEmpty())
        basename = "download";

    if (QFile::exists(path)) {
        // already exists, don't overwrite
        qInfo() << path << "already exists";
    }

    return rootFolder + "/" + path;
}

release_file Updater::parseReleaseFile(QStringList pars) {
    release_file file;
    file.path = pars.at(0);
    file.size = pars.at(1).toLong();
    file.timestamp = pars.at(2).toLong();
    file.hash = pars.at(3);
    file.isExe = pars.at(4).toInt();

    return file;
}

void Updater::downloadFinished(QNetworkReply *reply) {
    QUrl url = reply->url();
    QString path = url.path().replace("/caesium/" + osAndExtension.at(0) + "/", "");

    if (reply->error()) {
        fprintf(stderr, "Download of %s failed: %s\n",
                url.toEncoded().constData(),
                qPrintable(reply->errorString()));
    } else {
        if (hash.contains(reply)) {
            release_file file = hash.value(reply);
            QString filename = getDownloadPath(url);
            if (saveToDisk(filename, reply, &file)) {
                printf("Download of %s succeeded \n->\t%s\n",
                       url.toEncoded().constData(), qPrintable(filename));
                //Check integrity and permissions
                if (file.isExe == 1) {
                    qInfo() << "Setting exe permission to" << rootFolder + "/" + file.path;
                    QFile(rootFolder + "/" + file.path).setPermissions(QFile(rootFolder + "/" + file.path).permissions() | QFile::ExeGroup | QFile::ExeOther | QFile::ExeOther | QFile::ExeUser);
                }
            }
        }
    }
    currentDownloads.removeAll(reply);
    reply->deleteLater();

    if (currentDownloads.isEmpty()) {
        // all downloads finished
        qInfo() << "All download finished";
        releaseList.clear();
        if (!renameSelf()) {
            qCritical() << "Could not rename myself!";
        }
        //Copy the updated files
        if (!overwriteOriginal(rootFolder, QCoreApplication::applicationDirPath())) {
            qCritical() << "Could not overwrite old files";
        }
        QCoreApplication::instance()->quit();
    }
}

int Updater::compareChecksums(QString checksum, QByteArray* file) {
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(*file);
    QString downloadedChecksum = hash.result().toHex();
    qInfo() << "Comparing checksums. Original: " << checksum << "\nDownloaded: " << downloadedChecksum;
    return QString::compare(downloadedChecksum, checksum);
}

bool Updater::renameSelf() {
    qInfo() << "Renaming myself...";
    QFile self(QCoreApplication::applicationFilePath());
    QFile selfNew(QCoreApplication::applicationFilePath() + ".old");
    if (selfNew.exists()) {
        if (!selfNew.remove()) {
            qCritical() << "Cannot remove old updater";
            return false;
        }
    }
    return self.rename(selfNew.fileName());
}

bool Updater::overwriteOriginal(QString oldPath, QString newPath) {
    qDebug() << "Old path is:" << oldPath;
#ifdef __APPLE__
    QDir temp(newPath);
    temp.cd("../../..");
    newPath = temp.absolutePath();
#endif
    qDebug() << "New path is:" << newPath;
    return copyRecursively(oldPath, newPath);
}

bool Updater::copyRecursively(const QString &srcFilePath, const QString &tgtFilePath) {
    QFileInfo srcFileInfo(srcFilePath);
    if (srcFileInfo.isDir()) {
        QDir targetDir(tgtFilePath);
        if (!targetDir.cdUp()) {
            return false;
        }
        if (!targetDir.exists()) {
            if (!targetDir.mkdir(QFileInfo(tgtFilePath).fileName()))
                return false;
        }
        QDir sourceDir(srcFilePath);
        QStringList fileNames = sourceDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        foreach (const QString &fileName, fileNames) {
            const QString newSrcFilePath
                    = srcFilePath + QLatin1Char('/') + fileName;
            const QString newTgtFilePath
                    = tgtFilePath + QLatin1Char('/') + fileName;
            if (!copyRecursively(newSrcFilePath, newTgtFilePath))
                return false;
        }
    } else {
        if (QFile(tgtFilePath).exists()) {
            //TODO Add check
            QFile(tgtFilePath).remove();
        }
        if (!QFile::copy(srcFilePath, tgtFilePath))
            return false;
    }
    return true;
}
