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
    switch (type) {
    case QtDebugMsg:
        fprintf(stdout, "[%s][D] %s (%s:%u, %s)\n", formattedTime.toLocal8Bit().constData(), localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtInfoMsg:
        fprintf(stdout, "[%s][I] %s\n", formattedTime.toLocal8Bit().constData(), localMsg.constData());
        break;
    case QtWarningMsg:
        fprintf(stdout, "[%s][W] %s\n", formattedTime.toLocal8Bit().constData(), localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stdout, "[%s][C] %s (%s:%u, %s)\n", formattedTime.toLocal8Bit().constData(), localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stdout, "[%s][F] %s (%s:%u, %s)\n", formattedTime.toLocal8Bit().constData(), localMsg.constData(), context.file, context.line, context.function);
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
    QCoreApplication::setApplicationVersion("2.0.0-beta.3");

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
