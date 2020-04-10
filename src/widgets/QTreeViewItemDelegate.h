#ifndef QTREEVIEWITEMDELEGATE_H
#define QTREEVIEWITEMDELEGATE_H

#include <QStyledItemDelegate>
#include <QSvgRenderer>
#include <QTreeView>

class QTreeViewItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    QTreeViewItemDelegate(QTreeView* view, QObject* parent);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QSvgRenderer* svgRenderer;
    QTreeView* m_view;
};

#endif // QTREEVIEWITEMDELEGATE_H
