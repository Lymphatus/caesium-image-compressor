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
    QLocale locale = QLocale(localeId);
    if (localeId != "default" && translator->load(locale, QLatin1String("caesium"), QLatin1String("_"), QLatin1String(":/i18n"))) {
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
    QCoreApplication::setApplicationVersion("2.3.0-beta");

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
