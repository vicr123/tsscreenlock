#ifndef TIMERCOMPLETE_H
#define TIMERCOMPLETE_H

#include <QDialog>
#include <QPainter>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QFrame>
#include "tpropertyanimation.h"

namespace Ui {
class TimerComplete;
}

class TimerComplete : public QDialog
{
    Q_OBJECT

public:
    explicit TimerComplete(QRect geometry, QWidget *parent = 0);
    ~TimerComplete();

    void showFullScreen();

private:
    Ui::TimerComplete *ui;

    int pressLocation;

    void paintEvent(QPaintEvent* event);
    bool eventFilter(QObject *obj, QEvent *eve);

    QRect screenGeometry;
};

#endif // TIMERCOMPLETE_H
