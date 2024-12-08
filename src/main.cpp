#include "MainWindow.h"
#include "utils/Logger.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QUuid>

QString loadInstallationId()
{
    if (!QSettings().contains("uuid")) {
        QString newUUID = QUuid::createUuid().toString(QUuid::WithoutBraces);
        QSettings().setValue("uuid", newUUID);
        return newUUID;
    } else {
        return QSettings().value("uuid").toString();
    }
}

void loadTheme(QApplication& a)
{
    int themeIndex = QSettings().value("preferences/general/theme", 0).toInt();
    int themeVariantIndex = QSettings().value("preferences/general/theme_variant").toInt();
    if (themeVariantIndex == 1) {
        QApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    } else if (themeVariantIndex == 2) {
        QApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    }

    // From 2.4.1 and Qt 6.5, the framework can handle the switch between dark and light
    if (themeIndex > 1) {
        themeIndex = 1;
    }

#ifdef Q_OS_WIN
    if (themeIndex == 0) {
        a.setStyleSheet("QSplitter::handle:horizontal{image:url(:/icons/ui/v_handle.png);}QSplitter::handle:vertical{image:url(:/icons/ui/h_handle.png);}QStatusBar::item{border: none;}");
    }
#endif

    if (themeIndex == 1) {
        QApplication::setStyle(QStyleFactory::create(THEMES[themeIndex].theme));
    }
}

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName("SaeraSoft");
    QCoreApplication::setOrganizationDomain("saerasoft.com");
    QCoreApplication::setApplicationName("Caesium Image Compressor");
    QCoreApplication::setApplicationVersion("2.8.3");

#ifdef NDEBUG
    qInstallMessageHandler(Logger::messageHandler);
#endif

    QApplication a(argc, argv);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.process(a);

    loadTheme(a);

    qInfo() << "---- Starting application ----";
    QString uuid = loadInstallationId();
    qInfo() << "UUID:" << uuid;
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
