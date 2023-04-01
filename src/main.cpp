#include "MainWindow.h"
#include "utils/LanguageManager.h"
#include "utils/Logger.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QSettings>
#include <QStyleFactory>
#include <QTranslator>
#include <QUuid>

QLocale loadLocale(QTranslator* translator)
{
    QString localeId = LanguageManager::getLocaleFromPreferences(QSettings().value("preferences/language/locale", "default"));
    QLocale locale = QLocale();
    if (localeId != "default") {
        locale = QLocale(localeId);
    }
    if (translator->load(locale, QLatin1String("caesium"), QLatin1String("_"), QLatin1String(":/i18n"))) {
        QCoreApplication::installTranslator(translator);
    }

    return locale;
}

void loadInstallationId()
{
    if (!QSettings().contains("uuid")) {
        QSettings().setValue("uuid", QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
}

void loadTheme(QApplication* a)
{
    int themeIndex = QSettings().value("preferences/general/theme", 0).toInt();
    if (themeIndex > 0 && themeIndex < THEMES_COUNT) {
        QApplication::setStyle(QStyleFactory::create(THEMES[themeIndex].theme));

        if (themeIndex == 1) {
            QColor darkGray(25, 25, 25);
            //QColor gray(32, 32, 32);
            QColor lightGray(82, 82, 82);
            QColor black(0, 0, 0);
            QColor blue(37, 99, 235);
            QColor purple(168, 85, 247);
            QColor white(241, 245, 249);

            QPalette darkPalette;
            darkPalette.setColor(QPalette::Window, darkGray);
            darkPalette.setColor(QPalette::WindowText, white);
            darkPalette.setColor(QPalette::Base, black);
            darkPalette.setColor(QPalette::AlternateBase, darkGray);
            darkPalette.setColor(QPalette::ToolTipBase, blue);
            darkPalette.setColor(QPalette::ToolTipText, white);
            darkPalette.setColor(QPalette::Text, white);
            darkPalette.setColor(QPalette::Button, darkGray);
            darkPalette.setColor(QPalette::ButtonText, white);
            darkPalette.setColor(QPalette::Link, blue);
            darkPalette.setColor(QPalette::Highlight, purple);
            darkPalette.setColor(QPalette::HighlightedText, black);

            darkPalette.setColor(QPalette::Active, QPalette::Button, darkGray);
            darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, lightGray);
            darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, lightGray);
            darkPalette.setColor(QPalette::Disabled, QPalette::Text, lightGray);
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
    QCoreApplication::setApplicationVersion("2.4.0");

#ifdef NDEBUG
    qInstallMessageHandler(Logger::messageHandler);
#endif
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

    QScreen* spawnScreen = w.screen();
    if (!QGuiApplication::screens().contains(spawnScreen)) {
        QScreen* cursorScreen = QApplication::screenAt(QCursor::pos());
        w.move(cursorScreen->availableGeometry().center() - w.rect().center());
        w.setScreen(cursorScreen);
    } else if (!spawnScreen->availableGeometry().contains(w.pos())) {
        w.move(spawnScreen->availableGeometry().center() - w.rect().center());
    }

    w.show();

    return QApplication::exec();
}
