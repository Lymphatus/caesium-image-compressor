#ifndef NETWORKOPERATIONS_H
#define NETWORKOPERATIONS_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressDialog>

class NetworkOperations : public QObject
{
    Q_OBJECT

public:
    explicit NetworkOperations(QObject *parent = 0);
    void checkForUpdates();
    void downloadUpdateRequest();

public slots:
    void uploadUsageStatistics();
    void uploadFinished(QNetworkReply*);
    void getCurrentBuild();
    void showUpdateDownloadProgress(qint64, qint64);
    void flushUpdate(); 

signals:
    void checkForUpdatesFinished(int, QString);
    void updateDownloadFinished(QString);

private:
    QNetworkAccessManager* networkManager = new QNetworkAccessManager();
    QNetworkReply* updateReply;
    QNetworkReply* downloadUpdateReply;
    QString releaseURL;
    QProgressDialog* pDialog;
};

#endif // NETWORKOPERATIONS_H
