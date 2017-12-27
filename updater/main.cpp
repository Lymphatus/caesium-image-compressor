#include "updater.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Updater w;
    w.show();

    return a.exec();
}
