#include "MainWindow.h"

#include <QApplication>
#include <QDateTime>
#include <QStandardPaths>

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QDateTime currentTime = QDateTime::currentDateTime();
    QString formattedTime = currentTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "[%s][D] %s (%s:%u, %s)\n", formattedTime.toLocal8Bit().constData(), localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtInfoMsg:
        fprintf(stderr, "[%s][I] %s\n", formattedTime.toLocal8Bit().constData(), localMsg.constData());
        break;
    case QtWarningMsg:
        fprintf(stderr, "[%s][W] %s\n", formattedTime.toLocal8Bit().constData(), localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "[%s][C] %s (%s:%u, %s)\n", formattedTime.toLocal8Bit().constData(), localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "[%s][F] %s (%s:%u, %s)\n", formattedTime.toLocal8Bit().constData(), localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
}

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName("SaeraSoft");
    QCoreApplication::setOrganizationDomain("com.saerasoft.caesium");
    QCoreApplication::setApplicationName("Caesium Image Compressor");
    qInstallMessageHandler(messageHandler);
    QApplication a(argc, argv);

    qInfo() << "---- Starting application ----";
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    MainWindow w;
    w.show();

    return a.exec();
}
