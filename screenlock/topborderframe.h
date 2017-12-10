#ifndef TOPBORDERFRAME_H
#define TOPBORDERFRAME_H

#include <QFrame>
#include <QPainter>
#include <QPaintEvent>

class TopBorderFrame : public QFrame
{
    Q_OBJECT
public:
    explicit TopBorderFrame(QWidget *parent = 0);

signals:

public slots:

private:
    void paintEvent(QPaintEvent* event);
};

#endif // TOPBORDERFRAME_H
