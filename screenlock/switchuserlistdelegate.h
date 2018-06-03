#ifndef SWITCHUSERLISTDELEGATE_H
#define SWITCHUSERLISTDELEGATE_H

#include <QAbstractItemDelegate>
#include <QPainter>

class SwitchUserListDelegate : public QAbstractItemDelegate
{
    Q_OBJECT

public:
    explicit SwitchUserListDelegate(QObject* parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

#endif // SWITCHUSERLISTDELEGATE_H
