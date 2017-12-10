#include "topborderframe.h"

TopBorderFrame::TopBorderFrame(QWidget *parent) : QFrame(parent)
{

}

void TopBorderFrame::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setPen(this->palette().color(QPalette::WindowText));
    painter.drawLine(0, 0, this->width(), 0);
    event->accept();
}
