#include "QTreeViewItemDelegate.h"

/* Here for future use */

QTreeViewItemDelegate::QTreeViewItemDelegate(QTreeView* view, QObject* parent)
    : QStyledItemDelegate(parent)
    , m_view(view)
{
    svgRenderer = new QSvgRenderer(m_view);
    connect(svgRenderer, &QSvgRenderer::repaintNeeded, [this] {
        m_view->viewport()->update();
    });
}

void QTreeViewItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt { option };
    initStyleOption(&opt, index);

    if (index.column() == 0) {
        this->svgRenderer->load(QString(":/icons/compression_statuses/compressing.svg"));
        if (false) {
            auto bounds = opt.rect;
            bounds.setWidth(16);
            bounds.moveTo(2, opt.rect.center().y() - bounds.height() / 2);

            this->svgRenderer->render(painter, bounds);
        }
    }

    QStyledItemDelegate::paint(painter, opt, index);
}
