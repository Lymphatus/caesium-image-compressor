#ifndef CAESIUM_IMAGE_COMPRESSOR_LOGGER_H
#define CAESIUM_IMAGE_COMPRESSOR_LOGGER_H

#include <QtMessageHandler>
class Logger {
    static constexpr int RETENTION_DAYS = 30;

public:
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    static void cleanOldLogs();
    static QString getLogFilePath();
    static QString getLogDir();
    static void closeLogFile();
};

#endif // CAESIUM_IMAGE_COMPRESSOR_LOGGER_H
