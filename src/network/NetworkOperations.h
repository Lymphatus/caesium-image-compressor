#ifndef CAESIUM_IMAGE_COMPRESSOR_NETWORKOPERATIONS_H
#define CAESIUM_IMAGE_COMPRESSOR_NETWORKOPERATIONS_H

#include <QString>
#include <QNetworkAccessManager>
#include "utils/Utils.h"

class NetworkOperations : public QObject {
    Q_OBJECT
private:
    QNetworkAccessManager* networkClient;
    QString baseEndpoint;

    static QString getBaseEndpoint();
public:
    NetworkOperations();

    void requestToken();
    void updateSystemInfo();
    void sendUsageReport(CompressionSummary compressionSummary);
    virtual ~NetworkOperations();
};

#endif // CAESIUM_IMAGE_COMPRESSOR_NETWORKOPERATIONS_H
