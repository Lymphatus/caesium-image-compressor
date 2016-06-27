#include "src/updater.h"
#include <QCoreApplication>

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    QCoreApplication::setApplicationName("Caesium");
    QCoreApplication::setOrganizationName("SaeraSoft");
    QCoreApplication::setOrganizationDomain("saerasoft.com");

    qDebug() << "------START------";

    Updater* updater = new Updater();
    updater->requestReleaseList();

    return a.exec();
}
