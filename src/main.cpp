#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QSettings>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTranslator>
#include <QUuid>

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
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

QLocale loadLocale(QTranslator* translator)
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

    return locale;
}

void loadInstallationId()
{
    QSettings settings;

    if (!settings.contains("uuid")) {
        settings.setValue("uuid", QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
}

void loadTheme(QApplication* a)
{
    QSettings settings;

    int themeIndex = settings.value("preferences/general/theme", 0).toInt();
    if (themeIndex > 0 && themeIndex < THEMES_COUNT) {
        QApplication::setStyle(QStyleFactory::create(THEMES[themeIndex].theme));

        if (themeIndex == 1) {
            QColor darkGray(40, 40, 40);
            QColor gray(70, 70, 70);
            QColor black(25, 25, 25);
            QColor blue(99, 102, 241);
            QColor purple(147, 51, 234);

            QPalette darkPalette;
            darkPalette.setColor(QPalette::Window, darkGray);
            darkPalette.setColor(QPalette::WindowText, Qt::white);
            darkPalette.setColor(QPalette::Base, black);
            darkPalette.setColor(QPalette::AlternateBase, darkGray);
            darkPalette.setColor(QPalette::ToolTipBase, blue);
            darkPalette.setColor(QPalette::ToolTipText, Qt::white);
            darkPalette.setColor(QPalette::Text, Qt::white);
            darkPalette.setColor(QPalette::Button, darkGray);
            darkPalette.setColor(QPalette::ButtonText, Qt::white);
            darkPalette.setColor(QPalette::Link, blue);
            darkPalette.setColor(QPalette::Highlight, purple);
            darkPalette.setColor(QPalette::HighlightedText, Qt::black);

            darkPalette.setColor(QPalette::Active, QPalette::Button, gray.darker());
            darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
            darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
            darkPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
            darkPalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

            QApplication::setPalette(darkPalette);

            a->setStyleSheet("QToolTip { color: #ffffff; background-color: #404040; border: 1px solid darkgray; }");
        }
    }
}

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName("SaeraSoft");
    QCoreApplication::setOrganizationDomain("saerasoft.com");
    QCoreApplication::setApplicationName("Caesium Image Compressor");
    QCoreApplication::setApplicationVersion("2.2.0");

    qInstallMessageHandler(messageHandler);
    QApplication a(argc, argv);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.process(a);

    QTranslator translator;
    QLocale currentLocale = loadLocale(&translator);
    QApplication::setLayoutDirection(currentLocale.textDirection());
    loadTheme(&a);

    qInfo() << "---- Starting application ----";
    loadInstallationId();
    MainWindow w;
    w.show();

    return QApplication::exec();
}
