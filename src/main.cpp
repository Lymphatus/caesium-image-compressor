#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QStandardPaths>
#include <QTranslator>
#include <QSettings>
#include <QUuid>

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QDateTime currentTime = QDateTime::currentDateTime();
    QString formattedTime = currentTime.toString("yyyy-MM-dd hh:mm:ss.zzz");
    QByteArray localMsg = msg.toLocal8Bit();
    QFile logFile(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + "caesium.log");
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

    if (logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
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

void loadLocale(QTranslator* translator)
{
    QSettings settings;

    int localeIndex = settings.value("preferences/language/locale", 0).toInt();
    QLocale locale = QLocale();
    if (localeIndex < 0 || localeIndex > LANGUAGES_COUNT - 1) {
        localeIndex = 0;
    }
    if (localeIndex != 0) {
        locale = QLocale(LANGUAGES[localeIndex].locale);
    }
    if (translator->load(locale, QLatin1String("caesium"), QLatin1String("_"), QLatin1String(":/i18n"))) {
        QCoreApplication::installTranslator(translator);
    }
}

void loadInstallationId()
{
    QSettings settings;

    if (!settings.contains("uuid")) {
        settings.setValue("uuid", QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
}

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName("SaeraSoft");
    QCoreApplication::setOrganizationDomain("saerasoft.com");
    QCoreApplication::setApplicationName("Caesium Image Compressor");
    QCoreApplication::setApplicationVersion("2.0.0-beta.4");

    qInfo() << "Writing logs to" << QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + "caesium.log";
    qInstallMessageHandler(messageHandler);
    QApplication a(argc, argv);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.process(a);

    QTranslator translator;
    loadLocale(&translator);

    qInfo() << "---- Starting application ----";
    loadInstallationId();
    MainWindow w;
    w.show();

    return QApplication::exec();
}
