#ifndef CAESIUM_IMAGE_COMPRESSOR_HTMLDELEGATE_H
#define CAESIUM_IMAGE_COMPRESSOR_HTMLDELEGATE_H

#include <QStyledItemDelegate>

class HtmlDelegate : public QStyledItemDelegate {
protected:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};
#endif // CAESIUM_IMAGE_COMPRESSOR_HTMLDELEGATE_H
