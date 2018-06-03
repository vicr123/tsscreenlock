#include "switchuserlistdelegate.h"

extern float getDPIScaling();

SwitchUserListDelegate::SwitchUserListDelegate(QObject *parent) : QAbstractItemDelegate(parent) {

}


void SwitchUserListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    painter->setFont(option.font);

    QRect iconRect;
    iconRect.setLeft(option.rect.left() + 6 * getDPIScaling());
    iconRect.setTop(option.rect.top() + 6 * getDPIScaling());
    iconRect.setBottom(iconRect.top() + 32 * getDPIScaling());
    iconRect.setRight(iconRect.left() + 32 * getDPIScaling());

    QRect textRect;
    textRect.setLeft(iconRect.right() + 6 * getDPIScaling());
    textRect.setTop(option.rect.top() + 6 * getDPIScaling());
    textRect.setBottom(option.rect.top() + option.fontMetrics.height() + 6 * getDPIScaling());
    textRect.setRight(option.rect.right());

    QRect descRect;
    descRect.setLeft(iconRect.right() + 6 * getDPIScaling());
    descRect.setTop(option.rect.top() + option.fontMetrics.height() + 8 * getDPIScaling());
    descRect.setBottom(option.rect.top() + option.fontMetrics.height() * 2 + 6 * getDPIScaling());
    descRect.setRight(option.rect.right());

    if (option.state & QStyle::State_Selected) {
        painter->setPen(Qt::transparent);
        painter->setBrush(option.palette.color(QPalette::Highlight));
        painter->drawRect(option.rect);
        painter->setBrush(Qt::transparent);
        painter->setPen(option.palette.color(QPalette::HighlightedText));
        painter->drawText(textRect, index.data().toString());
        painter->drawText(descRect, index.data(Qt::UserRole + 1).toString());
    } else if (option.state & QStyle::State_MouseOver) {
        QColor col = option.palette.color(QPalette::Highlight);
        col.setAlpha(127);
        painter->setBrush(col);
        painter->setPen(Qt::transparent);
        painter->drawRect(option.rect);
        painter->setBrush(Qt::transparent);
        painter->setPen(option.palette.color(QPalette::WindowText));
        painter->drawText(textRect, index.data().toString());
        painter->setPen(option.palette.color(QPalette::Disabled, QPalette::WindowText));
        painter->drawText(descRect, index.data(Qt::UserRole + 1).toString());
    } else {
        painter->setPen(option.palette.color(QPalette::WindowText));
        painter->drawText(textRect, index.data().toString());
        painter->setPen(option.palette.color(QPalette::Disabled, QPalette::WindowText));
        painter->drawText(descRect, index.data(Qt::UserRole + 1).toString());
    }
    painter->drawPixmap(iconRect, index.data(Qt::DecorationRole).value<QIcon>().pixmap(32 * getDPIScaling(), 32 * getDPIScaling()));
}

QSize SwitchUserListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    int fontHeight = option.fontMetrics.height() * 2 + 14 * getDPIScaling();
    int iconHeight = 46 * getDPIScaling();

    return QSize(option.fontMetrics.width(index.data().toString()), qMax(fontHeight, iconHeight));
}
