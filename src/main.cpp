#include "MainWindow.h"
#include "utils/LanguageManager.h"
#include "utils/Logger.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
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
    // From 2.4.1 and Qt 6.5, the framework can handle the switch between dark and light
    if (themeIndex > 1) {
        themeIndex = 1;
    }

    if (themeIndex == 1) {
        QApplication::setStyle(QStyleFactory::create(THEMES[themeIndex].theme));
        auto palette = QApplication::palette();
        QColor purple(168, 85, 247);
        palette.setColor(QPalette::Highlight, purple);
        if (QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
            QColor darkGray(25, 25, 25);
            //QColor gray(32, 32, 32);
            QColor lightGray(82, 82, 82);
            QColor black(0, 0, 0);
            QColor blue(37, 99, 235);
            QColor white(241, 245, 249);

            palette.setColor(QPalette::Window, darkGray);
            palette.setColor(QPalette::WindowText, white);
            palette.setColor(QPalette::Base, black);
            palette.setColor(QPalette::AlternateBase, darkGray);
            palette.setColor(QPalette::ToolTipBase, blue);
            palette.setColor(QPalette::ToolTipText, white);
            palette.setColor(QPalette::Text, white);
            palette.setColor(QPalette::Button, darkGray);
            palette.setColor(QPalette::ButtonText, white);
            palette.setColor(QPalette::Link, blue);
            palette.setColor(QPalette::HighlightedText, black);

            palette.setColor(QPalette::Active, QPalette::Button, darkGray);
            palette.setColor(QPalette::Disabled, QPalette::ButtonText, lightGray);
            palette.setColor(QPalette::Disabled, QPalette::WindowText, lightGray);
            palette.setColor(QPalette::Disabled, QPalette::Text, lightGray);
            palette.setColor(QPalette::Disabled, QPalette::Light, darkGray);
        }
        QApplication::setPalette(palette);
    }
}

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName("SaeraSoft");
    QCoreApplication::setOrganizationDomain("saerasoft.com");
    QCoreApplication::setApplicationName("Caesium Image Compressor");
    QCoreApplication::setApplicationVersion("2.4.1");

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
