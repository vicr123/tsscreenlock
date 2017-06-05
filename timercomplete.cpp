#include "timercomplete.h"
#include "ui_timercomplete.h"

TimerComplete::TimerComplete(QRect geometry, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TimerComplete)
{
    ui->setupUi(this);

    ui->ArrowUp->setPixmap(QIcon::fromTheme("go-up").pixmap(16, 16));
    ui->slideOffFrame->installEventFilter(this);

    screenGeometry = geometry;
}

TimerComplete::~TimerComplete()
{
    delete ui;
}

void TimerComplete::showFullScreen() {
    this->setAttribute(Qt::WA_TranslucentBackground);

    this->setGeometry(screenGeometry);
    QDialog::showFullScreen();
    QApplication::processEvents();

    //this->layout()->removeWidget(ui->slideOffFrame);
    ui->slideOffFrame->setGeometry(0, this->height(), this->width(), ui->slideOffFrame->sizeHint().height());
    ui->slideOffFrame->setFixedHeight(ui->slideOffFrame->sizeHint().height());
    ui->slideOffFrame->setFixedWidth(this->width());

    tPropertyAnimation* anim = new tPropertyAnimation(ui->slideOffFrame, "geometry");
    anim->setStartValue(ui->slideOffFrame->geometry());
    anim->setEndValue(QRect(0, this->height() - ui->slideOffFrame->height(), this->width(), ui->slideOffFrame->height()));
    anim->setDuration(250);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
    anim->start();
}

void TimerComplete::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    painter.setBrush(QColor(0, 0, 0, 150));
    painter.setPen(QColor(0, 0, 0, 0));
    painter.drawRect(0, 0, this->width(), ui->slideOffFrame->y());

    QLinearGradient grad(0, ui->slideOffFrame->geometry().bottom(), 0, ui->slideOffFrame->geometry().bottom() + 20);
    grad.setColorAt(0, this->palette().color(QPalette::Window));
    grad.setColorAt(1, QColor(Qt::transparent));
    //painter.setBrush(this->palette().brush(QPalette::Window));
    painter.setBrush(grad);
    painter.drawRect(0, ui->slideOffFrame->geometry().bottom(), this->width(), ui->slideOffFrame->geometry().bottom() + 20);

    painter.setBrush(Qt::transparent);
    painter.drawRect(0, ui->slideOffFrame->geometry().bottom() + 10, this->width(), ui->slideOffFrame->height() + this->height());
}

bool TimerComplete::eventFilter(QObject *obj, QEvent *eve) {
    if (obj == ui->slideOffFrame) {
        if (eve->type() == QEvent::MouseButtonPress) {
            QMouseEvent* event = (QMouseEvent*) eve;
            pressLocation = event->y();
            return true;
        } else if (eve->type() == QEvent::MouseMove) {
            int top = QCursor::pos().y() - this->y() - pressLocation;
            //if (top < 0) top = 0;
            ui->slideOffFrame->move(0, top);
            this->update();
            return true;
        } else if (eve->type() == QEvent::MouseButtonRelease) {
            if (ui->slideOffFrame->y() < 50) {

                tPropertyAnimation* anim = new tPropertyAnimation(ui->slideOffFrame, "geometry");
                anim->setStartValue(ui->slideOffFrame->geometry());
                anim->setEndValue(QRect(0, -ui->slideOffFrame->height(), this->width(), ui->slideOffFrame->height()));
                anim->setDuration(250);
                anim->setEasingCurve(QEasingCurve::InCubic);
                connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
                connect(anim, SIGNAL(finished()), this, SLOT(update()));
                connect(anim, &tPropertyAnimation::finished, [=] {
                    this->accept();
                });
                anim->start();
            } else {
                tPropertyAnimation* anim = new tPropertyAnimation(ui->slideOffFrame, "geometry");
                anim->setStartValue(ui->slideOffFrame->geometry());
                anim->setEndValue(QRect(0, this->height() - ui->slideOffFrame->height(), this->width(), ui->slideOffFrame->height()));
                anim->setDuration(500);
                anim->setEasingCurve(QEasingCurve::OutBounce);
                connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
                connect(anim, SIGNAL(valueChanged(QVariant)), this, SLOT(update()));
                anim->start();
            }
            return true;
        } else if (eve->type() == QEvent::Paint) {
            QPaintEvent* event = (QPaintEvent*) eve;
            QPainter painter(ui->slideOffFrame);
            painter.setBrush(ui->slideOffFrame->palette().brush(QPalette::Window));
            painter.setPen(Qt::transparent);
            painter.drawRect(event->rect());
            painter.setPen(ui->slideOffFrame->palette().color(QPalette::WindowText));
            painter.drawLine(0, 0, ui->slideOffFrame->width(), 0);
        }
    }
    return false;
}
