#ifndef CAESIUM_IMAGE_COMPRESSOR_NETWORKOPERATIONS_H
#define CAESIUM_IMAGE_COMPRESSOR_NETWORKOPERATIONS_H

#include "utils/Utils.h"
#include <QNetworkAccessManager>
#include <QString>

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
    ~NetworkOperations() override;
};

#endif // CAESIUM_IMAGE_COMPRESSOR_NETWORKOPERATIONS_H
