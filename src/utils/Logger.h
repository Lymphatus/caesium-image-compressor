#ifndef CAESIUM_IMAGE_COMPRESSOR_LOGGER_H
#define CAESIUM_IMAGE_COMPRESSOR_LOGGER_H

#include <QtMessageHandler>
class Logger {
    static const int RETENTION_DAYS = 30;

public:
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    static void cleanOldLogs();
};

#endif // CAESIUM_IMAGE_COMPRESSOR_LOGGER_H
