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

void loadTheme()
{
    QSettings settings;

    int themeIndex = settings.value("preferences/general/theme", 0).toInt();
    if (themeIndex > 0 && themeIndex < THEMES_COUNT) {
        QApplication::setStyle(QStyleFactory::create(THEMES[themeIndex]));

#ifdef Q_OS_WIN
        if (themeIndex == 1) {
            QSettings s(R"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)", QSettings::NativeFormat);
            if (s.value("AppsUseLightTheme") == 0) {
                QPalette darkPalette;
                QColor darkColor = QColor(45, 45, 45);
                QColor disabledColor = QColor(127, 127, 127);
                darkPalette.setColor(QPalette::Window, darkColor);
                darkPalette.setColor(QPalette::WindowText, Qt::white);
                darkPalette.setColor(QPalette::Base, QColor(18, 18, 18));
                darkPalette.setColor(QPalette::AlternateBase, darkColor);
                darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
                darkPalette.setColor(QPalette::ToolTipText, Qt::white);
                darkPalette.setColor(QPalette::Text, Qt::white);
                darkPalette.setColor(QPalette::Disabled, QPalette::Text, disabledColor);
                darkPalette.setColor(QPalette::Button, darkColor);
                darkPalette.setColor(QPalette::ButtonText, Qt::white);
                darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledColor);
                darkPalette.setColor(QPalette::BrightText, Qt::red);
                darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));

                darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
                darkPalette.setColor(QPalette::HighlightedText, Qt::black);
                darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledColor);

                QApplication::setPalette(darkPalette);

                // a.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
            }
        }
#endif
    }
}

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName("SaeraSoft");
    QCoreApplication::setOrganizationDomain("saerasoft.com");
    QCoreApplication::setApplicationName("Caesium Image Compressor");
    QCoreApplication::setApplicationVersion("2.0.0");

    qInstallMessageHandler(messageHandler);
    QApplication a(argc, argv);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.process(a);

    QTranslator translator;
    loadLocale(&translator);
    loadTheme();

    qInfo() << "---- Starting application ----";
    loadInstallationId();
    MainWindow w;
    w.show();

    return QApplication::exec();
}
