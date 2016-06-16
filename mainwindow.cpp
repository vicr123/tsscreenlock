#include "ui_mainwindow.h"
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->incorrectPassword->setVisible(false);

    QString name = qgetenv("USER");
    if (name.isEmpty()) {
        name = qgetenv("USERNAME");
    }

    QProcess* fullNameProc = new QProcess(this);
    fullNameProc->start("getent passwd " + name);
    fullNameProc->waitForFinished();
    QString parseName(fullNameProc->readAll());
    delete fullNameProc;
    QString fullname = parseName.split(",").at(0).split(":").last();
    if (fullname == "" || fullname.contains("bash")) {
        ui->usernameLabel->setText(name);
    } else {
        ui->usernameLabel->setText(fullname);
    }

    //QRect screenGeometry = QApplication::desktop()->screenGeometry();
    ui->Cover->setParent(this);
    ui->Cover->setGeometry(0, 0, this->width(), this->height());

    QTimer* clockTimer = new QTimer();
    clockTimer->setInterval(1000);
    connect(clockTimer, &QTimer::timeout, [=]() {
        ui->dateLabel->setText(QDateTime::currentDateTime().toString("ddd dd MMM yyyy"));
        ui->timeLabel->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
        ui->dateLabel_2->setText(QDateTime::currentDateTime().toString("ddd dd MMM yyyy"));
        ui->timeLabel_2->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
    });
    clockTimer->start();

    ui->dateLabel->setText(QDateTime::currentDateTime().toString("ddd dd MMM yyyy"));
    ui->timeLabel->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
    ui->dateLabel_2->setText(QDateTime::currentDateTime().toString("ddd dd MMM yyyy"));
    ui->timeLabel_2->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));

    ui->downArrow->setPixmap(QIcon(":/icons/downarrowlight.svg").pixmap(32, 32));
    ui->upArrow->setPixmap(QIcon(":/icons/uparrowlight.svg").pixmap(32, 32));
}

MainWindow::~MainWindow()
{
    delete ui;
    XUngrabKeyboard(QX11Info::display(), CurrentTime);
    XUngrabPointer(QX11Info::display(), CurrentTime);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    ui->Cover->setGeometry(0, 0, this->width(), this->height());
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    hideCover();
}

void MainWindow::on_pushButton_clicked()
{
    ui->lineEdit->setEnabled(false);
    ui->pushButton->setEnabled(false);
    QString name = qgetenv("USER");
    if (name.isEmpty()) {
        name = qgetenv("USERNAME");
    }

    QProcess* tscheckpass = new QProcess();
    tscheckpass->start("tscheckpass " + name + " " + ui->lineEdit->text());
    tscheckpass->waitForFinished();
    if (tscheckpass->exitCode() == 0) {
        XUngrabKeyboard(QX11Info::display(), CurrentTime);
        XUngrabPointer(QX11Info::display(), CurrentTime);

        QApplication::exit();
    } else {
        QTime timer;
        timer.start();

        while (timer.elapsed() < 1000) {
            QApplication::processEvents();
        }

        ui->incorrectPassword->setVisible(true);
        ui->lineEdit->setText("");
    }
    delete tscheckpass;

    ui->lineEdit->setEnabled(true);
    ui->pushButton->setEnabled(true);
}

void MainWindow::on_lineEdit_returnPressed()
{
    if (typePassword) {
        ui->pushButton->click();
    } else {
        hideCover();
    }
}

void MainWindow::on_Cover_clicked()
{

    //ui->Cover->setVisible(false);
}

void MainWindow::on_Cover_MouseDown(QMouseEvent *event)
{
    this->moveY = event->y();
}

void MainWindow::on_Cover_MouseMove(QMouseEvent *event)
{
    QRect geometry = ui->Cover->geometry();
    geometry.translate(0, 0 - this->moveY + event->y());
    ui->Cover->setGeometry(geometry);
}

void MainWindow::on_Cover_MouseUp(QMouseEvent *event)
{
    hideCover();
}

void MainWindow::hideCover() {
    if (!typePassword) {
        QPropertyAnimation* animation = new QPropertyAnimation(ui->Cover, "geometry");
        animation->setStartValue(ui->Cover->geometry());
        if (ui->Cover->geometry().top() > 0) {
            animation->setEndValue(ui->Cover->geometry().translated(0, this->height()));
        } else {
            animation->setEndValue(ui->Cover->geometry().translated(0, -this->height()));
        }
        animation->setDuration(500);
        animation->setEasingCurve(QEasingCurve::OutCubic);
        animation->start();
        typePassword = true;
    }
}

void MainWindow::showCover() {
    if (typePassword) {
        QPropertyAnimation* animation = new QPropertyAnimation(ui->Cover, "geometry");
        animation->setStartValue(ui->Cover->geometry());
        animation->setEndValue(QRect(0, 0, this->width(), this->height()));
        animation->setDuration(500);
        animation->setEasingCurve(QEasingCurve::OutCubic);
        animation->start();
        typePassword = false;
    }
}

void MainWindow::on_lineEdit_textEdited(const QString &arg1)
{
    if (typePassword) {
        hideCover();
    }
}

void MainWindow::on_pushButton_3_clicked()
{
    showCover();
    QProcess::startDetached("xset dpms force off");
}

void MainWindow::on_pushButton_4_clicked()
{
    showCover();
    QList<QVariant> arguments;
    arguments.append(true);

    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", "Suspend");
    message.setArguments(arguments);
    QDBusConnection::systemBus().send(message);
}
