#pragma once
#include "TestImages.h"
#include <QThreadPool>
#include <QTimer>

// Worker signals are always delivered to this GUI-thread context through queued connections.
struct BatchFixture {
    TestImages::Corpus corpus;
    CImageTreeModel model;
    QObject context;
    QList<int> started, finished;
    QList<QPair<int, bool>> events;
    QThreadPool pool; // Destroy first: workers finish before model/corpus or captured lists disappear.
    BatchFixture(int n, int threads = 4, int width = 256, int height = 256)
    {
        pool.setMaxThreadCount(threads);
        model.appendItems(TestImages::makeImages(corpus.in, n, width, height), corpus.in.path());
        QObject::connect(&model, &CImageTreeModel::itemCompressionStarted, &context, [this](int row) {
            started << row; events << qMakePair(row, true);
        }, Qt::QueuedConnection);
        QObject::connect(&model, &CImageTreeModel::itemCompressionFinished, &context, [this](int row) {
            finished << row; events << qMakePair(row, false);
        }, Qt::QueuedConnection);
    }
};
