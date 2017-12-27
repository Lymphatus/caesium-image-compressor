#include "caesium.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Caesium w;
    w.show();

    //Retina fix
    a.setAttribute(Qt::AA_UseHighDpiPixmaps);

    return a.exec();
}
