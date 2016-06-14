#include "ui_mainwindow.h"
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    XGrabKeyboard(QX11Info::display(), RootWindow(QX11Info::display(), 0), True, GrabModeAsync, GrabModeAsync, CurrentTime);
    XGrabPointer(QX11Info::display(), RootWindow(QX11Info::display(), 0), True, None, GrabModeAsync, GrabModeAsync, RootWindow(QX11Info::display(), 0), None, CurrentTime);

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

    QTimer* clockTimer = new QTimer();
    clockTimer->setInterval(1000);
    connect(clockTimer, &QTimer::timeout, [=]() {
        ui->dateLabel->setText(QDateTime::currentDateTime().toString("ddd dd MMM yyyy"));
        ui->timeLabel->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
    });
    clockTimer->start();

    ui->dateLabel->setText(QDateTime::currentDateTime().toString("ddd dd MMM yyyy"));
    ui->timeLabel->setText(QDateTime::currentDateTime().toString("hh:mm:ss"));
}

MainWindow::~MainWindow()
{
    delete ui;
    XUngrabKeyboard(QX11Info::display(), CurrentTime);
    XUngrabPointer(QX11Info::display(), CurrentTime);
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
        this->close();
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
    ui->pushButton->click();
}
