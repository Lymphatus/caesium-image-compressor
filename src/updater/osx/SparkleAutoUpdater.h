/*
 * Copyright (C) 2008 Remko Troncon
 */

#ifndef SPARKLEAUTOUPDATER_H
#define SPARKLEAUTOUPDATER_H

#include <QString>

#include "AutoUpdater.h"

class SparkleAutoUpdater : public AutoUpdater {
public:
    SparkleAutoUpdater(const QString& url);
    ~SparkleAutoUpdater();

    void checkForUpdates();
    void setCheckForUpdatesAutomatically(bool check);

private:
    class Private;
    Private* d;
};

#endif
