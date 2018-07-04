#include "caesium.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("Caesium");
    QCoreApplication::setOrganizationName("SaeraSoft");
    QCoreApplication::setOrganizationDomain("saerasoft.com");

    Caesium w;
    w.show();

    //Retina fix
    a.setAttribute(Qt::AA_UseHighDpiPixmaps);

    return a.exec();
}
