#ifndef CAESIUM_IMAGE_COMPRESSOR_DATABASE_H
#define CAESIUM_IMAGE_COMPRESSOR_DATABASE_H

#include <QSqlDatabase>
class Database {
public:
    Database();

private:
    void init();

    QSqlDatabase db;
    int dbVersion = 1;
};

#endif // CAESIUM_IMAGE_COMPRESSOR_DATABASE_H
