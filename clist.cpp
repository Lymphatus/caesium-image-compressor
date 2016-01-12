#include "clist.h"
#include "utils.h"

#include <QFile>
#include <QDebug>

CList::CList() {

}

QList<QTreeWidgetItem*> CList::readFile(QString path) {
    //Create a QFile to perform some checks
    QFile in(path);
    //And the container list
    QList<QTreeWidgetItem*> items;
    //Check existance and open it
    if (in.exists() && in.open(QIODevice::ReadOnly)) {
        //TODO it does not check if the file is poorly written
        //First line is the file version number
        int version = QString(in.readLine()).split("\n").at(0).toInt();
        //If version is lesser than the current, perform version update
        if (version < CLF_VERSION) {
            //TODO Implement version update according to the future <3
        }

        //Second line is the column count
        int column_count = QString(in.readLine()).split("\n").at(0).toInt();

        //Third is the item count
        int item_count = QString(in.readLine()).split("\n").at(0).toInt();

        //Start reading data
        for (int i = 0; i < item_count; i++) {

            //Temporary list
            QStringList buffer;
            //For every item, read the columns
            for (int j = 0; j < column_count; j++) {
                buffer.append(QString(in.readLine()).split("\n").at(0));
            }
            //Add the compiled specs to the Items list
            items.append(new QTreeWidgetItem(buffer));
        }
        //CLose the file
        in.close();
    } else {
        qDebug() << "ERROR Failed to open " << path;
    }
    return items;
}

void CList::writeToFile(QList<QTreeWidgetItem *> list, QString path) {
    //Create a QFile to perform some checks
    QFile out(path);

    if (out.open(QIODevice::WriteOnly)) {
        //Fisrt line is the version number
        out.write(QByteArray::number(CLF_VERSION) + "\n");

        //Second is the columns number
        out.write(QByteArray::number(MAX_COLUMNS) + "\n");

        //Third is the item count
        out.write(QByteArray::number(list.length()) + "\n");

        //Then we can write the lines
        for (int i = 0; i < list.length(); i++) {
            for (int j = 0; j < MAX_COLUMNS; j++) {
                out.write(list.at(i)->text(j).toUtf8() + "\n");
            }
        }
        //CLose the file
        out.close();
    } else {
        qDebug() << "ERROR Failed to write " << path;
        return;
    }
}
