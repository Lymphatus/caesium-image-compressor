#include <climits>
#ifndef QDROPLISTWIDGET_H
#define QDROPLISTWIDGET_H

#include <QListWidget>

class QMimeData;

class QDropListWidget : public QListWidget {
    Q_OBJECT

public:
    explicit QDropListWidget(QWidget* parent = nullptr);

signals:
    void dropFinished(QDropEvent* event);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
};

#endif // QDROPLISTWIDGET_H
