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


    QSettings settings;
    QString background = settings.value("background", "/usr/share/icons/theos/backgrounds/triangle/1920x1080.png").toString();
    QPixmap backgroundImage(background);
    imageSize = backgroundImage.size();

    QImage image(background);
    QList<bool> lightOrDark;
    for (int w = 0; w < image.width(); w++) {
        for (int h = 0; h < image.height(); h++) {
            QColor col = image.pixelColor(w, h);
            int grey = (col.red() + col.green() + col.blue()) / 3;
            if (grey > 127) {
                lightOrDark.append(true);
            } else {
                lightOrDark.append(false);
            }
        }
    }

    QPalette pal;
    if (lightOrDark.count(true) <= lightOrDark.count() / 2) {
        pal.setColor(QPalette::WindowText, QColor::fromRgb(255, 255, 255));
        ui->downArrow->setPixmap(QIcon(":/icons/downarrowlight.svg").pixmap(32, 32));
        ui->upArrow->setPixmap(QIcon(":/icons/uparrowlight.svg").pixmap(32, 32));
    } else {
        pal.setColor(QPalette::WindowText, QColor::fromRgb(0, 0, 0));
        ui->downArrow->setPixmap(QIcon(":/icons/downarrowdark.svg").pixmap(32, 32));
        ui->upArrow->setPixmap(QIcon(":/icons/uparrowdark.svg").pixmap(32, 32));
    }
    ui->Cover->setPalette(pal);
    ui->dateLabel_2->setPalette(pal);
    ui->timeLabel_2->setPalette(pal);

    ui->ImageLabel->setParent(ui->Cover);
    ui->ImageLabel->setPixmap(backgroundImage);
    ui->ImageLabel->setGeometry(0, 0, this->width(), this->height());
    ui->ImageLabel->lower();

    /*ui->Cover->setStyleSheet("QWidget#Cover {"
                             "background: url(\"" + background + "\");"
                             "background-position: center;"
                             "}");*/

    QProcess* tscheckpass = new QProcess();
    tscheckpass->start("tscheckpass " + name);
    tscheckpass->waitForFinished();
    if (tscheckpass->exitCode() == 0) {
        ui->lockFrame->setVisible(false);
        ui->pushButton_3->setVisible(false);
        ui->pushButton_4->setVisible(false);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    XUngrabKeyboard(QX11Info::display(), CurrentTime);
    XUngrabPointer(QX11Info::display(), CurrentTime);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    ui->Cover->setGeometry(0, 0, this->width(), this->height());

    if (imageSize.width() < imageSize.height()) {
        ui->ImageLabel->resize(this->width(), this->width() / ((float) imageSize.width() / (float) imageSize.height()));
        ui->ImageLabel->move(0, this->height() / 2 - ui->ImageLabel->height() / 2);
    } else {
        ui->ImageLabel->resize(this->height() * ((float) imageSize.width() / (float) imageSize.height()), this->height());
        ui->ImageLabel->move(this->width() / 2 - ui->ImageLabel->width() / 2, 0);
    }

    //ui->ImageLabel->setGeometry(0, 0, this->width(), this->height());
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
        connect(animation, &QPropertyAnimation::finished, [=]() {
            QString name = qgetenv("USER");
            if (name.isEmpty()) {
                name = qgetenv("USERNAME");
            }

            QProcess* tscheckpass = new QProcess();
            tscheckpass->start("tscheckpass " + name);
            tscheckpass->waitForFinished();
            if (tscheckpass->exitCode() == 0) {
                QApplication::exit(0);
            }
        });
        typePassword = true;
        ui->lineEdit->setFocus();
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
        ui->lineEdit->setText("");
        ui->lineEdit->setFocus();
    }
}

void MainWindow::on_lineEdit_textEdited(const QString &arg1)
{
    if (!typePassword) {
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

void MainWindow::on_goBack_clicked()
{
    showCover();
}
