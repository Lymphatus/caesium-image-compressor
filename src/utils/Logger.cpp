#include "Logger.h"
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

void Logger::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QDateTime currentTime = QDateTime::currentDateTime();
    QString formattedTime = currentTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString currentDate = currentTime.toString("yyyy-MM-dd");
    QString logDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString logPath = logDirPath + "/caesium-" + currentDate + ".log";

    QByteArray localMsg = msg.toLocal8Bit();
    QDir logDir(logDirPath);
    bool logToFile = true;
    if (!logDir.exists()) {
        logToFile = logDir.mkpath(logDirPath);
    }
    QFile logFile(logPath);
    QString message;
    switch (type) {
    case QtDebugMsg:
        message = QString("[%1][D] %2 (%3:%4, %5)\n").arg(formattedTime.toLocal8Bit().constData(), localMsg.constData(), context.file, QString::number(context.line), context.function);
        break;
    case QtInfoMsg:
        message = QString("[%1][I] %2\n").arg(formattedTime.toLocal8Bit().constData(), localMsg.constData());
        break;
    case QtWarningMsg:
        message = QString("[%1][W] %2\n").arg(formattedTime.toLocal8Bit().constData(), localMsg.constData());
        break;
    case QtCriticalMsg:
        message = QString("[%1][C] %2 (%3:%4, %5)\n").arg(formattedTime.toLocal8Bit().constData(), localMsg.constData(), context.file, QString::number(context.line), context.function);
        break;
    case QtFatalMsg:
        message = QString("[%1][F] %2 (%3:%4, %5)\n").arg(formattedTime.toLocal8Bit().constData(), localMsg.constData(), context.file, QString::number(context.line), context.function);
        break;
    }

    logToFile = logFile.open(QIODevice::WriteOnly | QIODevice::Append) && logToFile;
    if (logToFile) {
        QTextStream log(&logFile);
        log << message;
        logFile.close();
    } else {
        fprintf(stdout, "%s", message.toLocal8Bit().constData());
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

void Logger::cleanOldLogs()
{
    QStringList filters;
    filters << "caesium-*.log";
    QFileInfoList fileInfoList = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).entryInfoList(filters, QDir::Files, QDir::Time);
    QDateTime now = QDateTime::currentDateTime();

    for (const QFileInfo& fileInfo : fileInfoList) {
        if (abs(fileInfo.lastModified().daysTo(now)) > RETENTION_DAYS) {
            QFile::remove(fileInfo.canonicalFilePath());
        }
    }
}
