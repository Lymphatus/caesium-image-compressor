#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QStandardPaths>

typedef struct release_file {
    QString path;
    long size;
    long timestamp;
    QString hash;
    int isExe;
} release_file;

class Updater : public QObject
{
    Q_OBJECT
public:
    explicit Updater(QObject *parent = 0);
    void requestReleaseList();
    void requestFileDownload(release_file);
    bool saveToDisk(const QString &filename, QIODevice *data, release_file *file);
    QString getDownloadPath(QUrl url);
    release_file parseReleaseFile(QStringList);
    int compareChecksums(QString checksum, QByteArray *file);
    bool renameSelf();
    bool overwriteOriginal(QString oldPath, QString newPath);
    bool copyRecursively(const QString &srcFilePath, const QString &tgtFilePath);
signals:

public slots:
    void getReleaseFiles();
    void downloadFinished(QNetworkReply *reply);
private:
    QString rootUrl;
    QString rootFolder;
    QList<release_file> releaseList;
    QHash<QNetworkReply*, release_file> hash;
    QNetworkAccessManager* networkManager;
    QNetworkReply* fileListReply;
    QList<QNetworkReply *> currentDownloads;
    QString rootDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QStringList osAndExtension = QStringList() <<
            #ifdef _WIN32
                "win" << ".exe";
            #elif __APPLE__
                "osx" << ".dmg";
            #else
                "linux" << ".tar.gz";
            #endif
};

#endif // UPDATER_H
