#ifndef UPDATER_H
#define UPDATER_H

#include <QFile>
#include <QNetworkReply>
#include <QString>
#include <QThread>

class Updater : public QObject {
    Q_OBJECT


private:
    size_t currentEndpointIndex = 0;
    QStringList updateEndpoints;
    QNetworkReply* downloadUpdateReply;
    QFile* updateFile;
    QString remoteUpdateHash;

    void sendCheckForUpdateRequest(const QString& endpoint);
    void retryCheckForUpdates();
    void parseRemoteMetadata(const QJsonDocument& jsonDocument);
    void downloadUpdateData(const QString& endpoint);
    void cleanup(bool aborted);

public:
    Updater();
    ~Updater();

    void checkForUpdates();
    void checkForUpdates(const QString& endpoint);
    static void replaceCurrentFiles(const QString& filePath);

signals:
    void resultReady(QString fileName);
    void finished();

private slots:
    void checkForUpdatesFinished(QNetworkReply* reply);
    void updateDownloadFinished();
    void updateDataReadReady();
    void downloadUpdateErrorOccurred(QNetworkReply::NetworkError);
};
#endif // UPDATER_H
