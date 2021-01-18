#include "Updater.h"
#include "vendor/zip/src/zip.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>

Updater::Updater()
    : QObject()
{
    updateEndpoints << "https://saerasoft.com/repository/" + QCoreApplication::organizationDomain() + "/" + QSysInfo::productType() + "/"
                    << "http://saerasoft.test/repository/" + QCoreApplication::organizationDomain() + "/" + QSysInfo::productType() + "/";

    updateFile = new QFile(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/com.saerasoft.caesium.zip");
    downloadUpdateReply = nullptr;
}

Updater::~Updater()
{
    this->cleanup(false);

    delete downloadUpdateReply;
    delete updateFile;
}

void Updater::sendCheckForUpdateRequest(const QString& endpoint)
{
    auto* manager = new QNetworkAccessManager();
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(checkForUpdatesFinished(QNetworkReply*)));

    QString jsonVersionEndpoint = endpoint + "updates.json";
    qInfo() << "[UPDATER] Checking for updates at" << jsonVersionEndpoint;
    manager->get(QNetworkRequest(QUrl(jsonVersionEndpoint)));
}

int on_extract_entry(const char* filename, void* arg)
{
    return 0;
}

void Updater::checkForUpdates()
{
    return this->sendCheckForUpdateRequest(updateEndpoints[currentEndpointIndex]);
}

void Updater::checkForUpdates(const QString& endpoint)
{
    return this->sendCheckForUpdateRequest(endpoint);
}

void Updater::checkForUpdatesFinished(QNetworkReply* reply)
{
    auto replyError = reply->error();
    if (replyError == QNetworkReply::NoError) {
        auto responseData = reply->readAll();
        reply->deleteLater();

        QJsonParseError* parseError = nullptr;
        QJsonDocument jsonDocument = QJsonDocument::fromJson(responseData, parseError);
        if (jsonDocument.isNull()) {
            qWarning() << "[UPDATER] Failed to parse JSON data from server. Error:" << parseError;
            emit finished();
            return;
        }

        return this->parseRemoteMetadata(jsonDocument);

    } else {
        // We should continue trying with all endpoints
        qWarning() << "[UPDATER] Update request returned error:" << replyError;

        reply->deleteLater();
        return this->retryCheckForUpdates();
    }
}

void Updater::retryCheckForUpdates()
{
    size_t endpointsCount = updateEndpoints.size();
    if (currentEndpointIndex >= endpointsCount - 1) {
        qWarning() << "[UPDATER] Max endpoints count reached. Aborting check for updates.";
        emit finished();
        return;
    }

    currentEndpointIndex++;
    return this->checkForUpdates(updateEndpoints[currentEndpointIndex]);
}

void Updater::parseRemoteMetadata(const QJsonDocument& jsonDocument)
{
    QJsonObject jsonObject = jsonDocument.object();

    if (!jsonObject.contains("version") || !jsonObject["version"].isString()) {
        qWarning() << "[UPDATER] JSON does not contain latestVersion data. Aborting.";
        emit finished();
        return;
    }

    if (!jsonObject.contains("hash") || !jsonObject["hash"].isString()) {
        qWarning() << "[UPDATER] JSON does not contain hash data. Aborting.";
        emit finished();
        return;
    }

    QString latestVersion = jsonObject.value("version").toString();

    if (latestVersion.compare(QCoreApplication::applicationVersion()) <= 0) {
        qInfo() << "[UPDATER] Current version is latest.";
        emit finished();
        return;
    }

    remoteUpdateHash = jsonObject.value("hash").toString();

    QString updatesEndpoint = updateEndpoints[currentEndpointIndex] + latestVersion + "/com.saerasoft.caesium.zip";
    this->downloadUpdateData(updatesEndpoint);
}

void Updater::downloadUpdateData(const QString& endpoint)
{
    qDebug() << "[UPDATER] Starting download from" << endpoint;

    if (updateFile->exists()) {
        this->updateDownloadFinished();
        return;
    }

    auto* manager = new QNetworkAccessManager();
    downloadUpdateReply = manager->get(QNetworkRequest(QUrl(endpoint)));
    connect(downloadUpdateReply, SIGNAL(readyRead()), this, SLOT(updateDataReadReady()));
    connect(downloadUpdateReply, SIGNAL(finished()), this, SLOT(updateDownloadFinished()));
    connect(downloadUpdateReply, SIGNAL(errorOccurred(QNetworkReply::NetworkError)), this, SLOT(downloadUpdateErrorOccurred(QNetworkReply::NetworkError)));
}

void Updater::updateDataReadReady()
{
    if (!updateFile->isOpen() && !updateFile->open(QIODevice::ReadWrite)) {
        qCritical() << "[UPDATER] Cannot open " << updateFile->fileName();
        return;
    }

    auto data = downloadUpdateReply->readAll();
    updateFile->write(data);
}

void Updater::downloadUpdateErrorOccurred(QNetworkReply::NetworkError error)
{
    qWarning() << "[UPDATER] Update download failed. Error:" << error;
}

void Updater::updateDownloadFinished()
{
    qDebug() << "[UPDATER] Download finished. File:" << updateFile->fileName();
    if (this->downloadUpdateReply != nullptr && this->downloadUpdateReply->error() != QNetworkReply::NoError) {
        emit finished();
        return;
    }

    if (!updateFile->isOpen()) {
        updateFile->open(QIODevice::ReadOnly);
    }
    QCryptographicHash localHash(QCryptographicHash::Sha512);
    localHash.addData(updateFile->readAll());

    QString localHashHex = localHash.result().toHex();

    qDebug() << "[UPDATER] L:" << localHashHex << " - R:" << remoteUpdateHash;

    if (localHashHex != remoteUpdateHash) {
        qCritical() << "[UPDATER] Downloaded file hash is different. Aborting.";
        this->cleanup(true);
        emit finished();
        return;
    }

    emit finished();
    emit resultReady(this->updateFile->fileName());
}

void Updater::cleanup(bool aborted)
{
    if (downloadUpdateReply != nullptr && downloadUpdateReply->isRunning()) {
        downloadUpdateReply->close();
    }

    if (updateFile->isOpen()) {
        updateFile->close();
    }

    if (aborted) {
        updateFile->remove();
    }
}

#ifdef __APPLE__
void Updater::replaceCurrentFiles(const QString& filePath)
{
    //TODO error handling
    QString tempSuffix = ".cstmp";
    QDir installDir = QDir(QCoreApplication::applicationDirPath());
    installDir.cd("../..");
    QFile bundleFile(installDir.absolutePath());
    QFile tempBundleFile(installDir.absolutePath() + tempSuffix);
    installDir.cd("..");
    QString originalBundleFilename = bundleFile.fileName();

    qDebug() << tempBundleFile.fileName();
    if (tempBundleFile.exists()) {
        QDir(tempBundleFile.fileName()).removeRecursively();
    }

    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    int arg = 2;
    int extractResult = zip_extract(filePath.toUtf8().constData(), tempPath.toUtf8().constData(), on_extract_entry, &arg);

    if (extractResult < 0) {
        qCritical() << "[UPDATER] Extract failed.";
        bundleFile.rename(originalBundleFilename);
        return;
    }

    bool renameSuccess = bundleFile.rename(tempBundleFile.fileName());

    if (!renameSuccess) {
        qCritical() << "[UPDATER] Bundle rename failed.";
        return;
    }

    QFile(tempPath + "/Caesium Image Compressor.app").copy(originalBundleFilename);

}
#else
void Updater::replaceCurrentFiles()
{

}
#endif
