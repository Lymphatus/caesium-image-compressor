#include "NetworkOperations.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QSettings>
#include <QStandardPaths>
#include "utils/Utils.h"

NetworkOperations::NetworkOperations()
{
    QSettings settings;
    this->networkClient = new QNetworkAccessManager();
    this->baseEndpoint = getBaseEndpoint();
}

NetworkOperations::~NetworkOperations()
{
    delete networkClient;
}

QString NetworkOperations::getBaseEndpoint()
{
    QFile endpointFile(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/api_endpoint");
    if (endpointFile.exists() && endpointFile.open(QFile::ReadOnly)) {
        return endpointFile.readLine();
    }
    return "https://caesium.app/api/v1";
}

void NetworkOperations::sendUsageReport(CompressionSummary compressionSummary)
{
    QSettings settings;
    if (!settings.contains("access_token")) {
        return;
    }
    QJsonObject requestData {
        { "uuid", settings.value("uuid").toString() },
        { "totalFiles", (double)compressionSummary.totalImages },
        { "uncompressedSize", compressionSummary.totalUncompressedSize },
        { "compressedSize", compressionSummary.totalCompressedSize },
        { "elapsedTime", compressionSummary.elapsedTime },
    };
    QNetworkRequest request(QUrl(this->baseEndpoint + "/desktopInstallation/summary"));
    QString bearerToken = "Bearer " + settings.value("access_token").toString();
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    request.setRawHeader("Authorization", bearerToken.toUtf8());
    request.setRawHeader("Accept", "application/json");
    QNetworkReply* reply = this->networkClient->post(request, QJsonDocument(requestData).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Usage report request failed. Error:" << reply->errorString() << "- Reply:" << reply->readAll();
        } else {
            qInfo() << "Usage report sent successfully";
        }
        reply->deleteLater();
    });
}

void NetworkOperations::requestToken()
{
    QSettings settings;
    if (!settings.contains("uuid")) {
        return;
    }
    QString uuid = settings.value("uuid").toString();
    if (uuid.isEmpty()) {
        return;
    }
    QNetworkRequest request(QUrl(this->baseEndpoint + "/tokens/create"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    QJsonObject requestData = getSystemData();

    QNetworkReply* reply = this->networkClient->post(request, QJsonDocument(requestData).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonParseError parseError;
            QSettings sts;
            QJsonDocument dataJsonDocument = QJsonDocument::fromJson(reply->readAll(), &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                qWarning() << "Cannot parse request token response. Error:" << parseError.errorString();
            } else {
                QString token = dataJsonDocument.object().value("token").toString();
                qInfo() << "Access token:" << token;
                sts.setValue("access_token", token);
            }
        } else {
            qWarning() << "Access token request failed. Error:" << reply->errorString() << "- Reply:" << reply->readAll();
        }
        reply->deleteLater();
    });
}

void NetworkOperations::updateSystemInfo()
{
    QSettings settings;
    QJsonObject requestData = getSystemData();
    QNetworkRequest request(QUrl(this->baseEndpoint + "/desktopInstallation/refresh"));
    QString bearerToken = "Bearer " + settings.value("access_token").toString();
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Authorization", bearerToken.toUtf8());
    QNetworkReply* reply = this->networkClient->post(request, QJsonDocument(requestData).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Sys info update failed. Error:" << reply->errorString() << "- Reply:" << reply->readAll();
        } else {
            qInfo() << "Sys info updated successfully";
        }
        reply->deleteLater();
    });
}
