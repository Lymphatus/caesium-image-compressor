#ifndef USAGEINFO_H
#define USAGEINFO_H

#include <QString>
#include <QTime>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>

#define USAGE_INFO_FILENAME QString("usageinfo.ph")

class UsageInfo
{

public:
    UsageInfo();

    QString jsonPath;

    void writeJSON();
    void readJSON();
    QString printJSON();
    void initialize();

    QString UUID;
    qint64 timestamp;

    QString productName;
    QString locale;
    QString arch;
    int appVersion;
    int build;

    unsigned long long compressed_bytes;
    unsigned int compressed_pictures;
    qint64 max_bytes;
    double best_ratio;


    void setCompressed_bytes(unsigned long long value);
    void setCompressed_pictures(unsigned int value);
    void setMax_bytes(unsigned int value);
    void setBest_ratio(double value);

private:

};

#endif // USAGEINFO_H
