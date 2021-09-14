/*
 * Copyright (C) 2008 Remko Troncon
 */

#include "SparkleAutoUpdater.h"

#include <Cocoa/Cocoa.h>
#include <Sparkle.h>

class SparkleAutoUpdater::Private
{
	public:
		SUUpdater* updater;
};

SparkleAutoUpdater::SparkleAutoUpdater(const QString& aUrl)
{
	d = new Private;

	d->updater = [SUUpdater sharedUpdater];
	[d->updater retain];

	NSURL* url = [NSURL URLWithString:
			[NSString stringWithUTF8String: aUrl.toUtf8().data()]];
	[d->updater setFeedURL: url];
}

SparkleAutoUpdater::~SparkleAutoUpdater()
{
	[d->updater release];
	delete d;
}

void SparkleAutoUpdater::checkForUpdates()
{
	[d->updater checkForUpdatesInBackground];
}

void SparkleAutoUpdater::setCheckForUpdatesAutomatically(bool check)
{
    [d->updater setAutomaticallyChecksForUpdates: check];
}
