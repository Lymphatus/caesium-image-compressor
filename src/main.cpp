#include "MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName("SaeraSoft");
    QCoreApplication::setOrganizationDomain("com.saerasoft.caesium");
    QCoreApplication::setApplicationName("Caesium Image Compressor");
    QApplication a(argc, argv);

    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    MainWindow w;
    w.show();

    return a.exec();
}
