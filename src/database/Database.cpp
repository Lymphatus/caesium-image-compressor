#include "Database.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

Database::Database()
{
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/caesium.sqlite";
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qCritical() << "Cannot connect to db DB at" << dbPath << "Error:" << db.lastError();
    }

    init();
}

void Database::init()
{
    QSqlQuery createCompressedImagesTable("create table if not exists compressed_images(id integer not null constraint compressed_images_pk primary key autoincrement, path_hash TEXT not null, size INT not null, image_hash TEXT not null, parameters_hash TEXT not null, db_version INT not null);");
    bool result = createCompressedImagesTable.exec();
    if (!result) {
        qCritical() << "Cannot create 'compressed_images' table. Error:" << db.lastError();
    }
}
