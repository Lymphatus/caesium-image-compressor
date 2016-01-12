#include "caesium.h"
#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <QLibraryInfo>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Caesium w;
    w.show();

    //2x images for OSX Retina
    a.setAttribute(Qt::AA_UseHighDpiPixmaps);

    //Load styles
#ifdef __APPLE__
    QFile qss(":/qss/style_osx.qss");
#elif _WIN32
    QFile qss(":/qss/style_win.qss");
#else
    QFile qss(":/qss/style_linux.qss");
#endif

    qss.open(QFile::ReadOnly);
    //Apply
    QString style(qss.readAll());
    a.setStyleSheet(style);

    //Translation support
    //QT Widgets
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(),
                      QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(&qtTranslator);

    //App translations
    QTranslator myappTranslator;
    myappTranslator.load("caesiumph_" + QLocale::system().name(),
                         QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    qDebug() << QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    a.installTranslator(&myappTranslator);

    return a.exec();
}
