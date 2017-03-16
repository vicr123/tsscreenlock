#include "ui_mainwindow.h"
#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->incorrectPassword->setVisible(false);
    QDBusConnection::sessionBus().connect("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "NotificationClosed", this, SLOT(closeNotification(uint,uint)));

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
    ui->timerFrame->setParent(this);
    ui->timerFrame->setGeometry(0, this->height(), this->width(), this->height());
    ui->hudFrame->setParent(this);
    ui->hudFrame->setGeometry(0, -ui->hudFrame->height(), this->width(), ui->hudFrame->sizeHint().height());
    ui->hudFrame->setFixedSize(this->width(), ui->hudFrame->sizeHint().height());

    //Set the menu of the MPRIS Media Player selection to a new menu.
    //Items will be populated during the MPRIS update event.
    QMenu* mprisSelectionMenu = new QMenu();
    ui->mprisSelection->setMenu(mprisSelectionMenu);
    connect(mprisSelectionMenu, &QMenu::aboutToShow, [=]() {
        pauseMprisMenuUpdate = true;
    });
    connect(mprisSelectionMenu, &QMenu::aboutToHide, [=]() {
        pauseMprisMenuUpdate = false;
    });

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
    ui->notificationScroll->setVisible(false);

    QSettings settings;
    QString background = settings.value("background", "/usr/share/icons/theos/backgrounds/triangle/1920x1080.png").toString();
    darkImage = QImage(background);

    for (int y = 0; y < darkImage.height(); y++) {
        for (int x = 0; x < darkImage.width(); x++) {
            darkImage.setPixelColor(x, y, darkImage.pixelColor(x, y).darker());
        }
    }

    imageSize = darkImage.size();

    image = QImage(background);
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
    ui->ImageLabel->setPixmap(QPixmap::fromImage(image));
    ui->ImageLabel->setGeometry(0, 0, this->width(), this->height());
    ui->ImageLabel->lower();

    ui->mprisFrame->setParent(this);
    ui->mprisFrame->setGeometry(0, this->height() - ui->mprisFrame->height(), this->width(), ui->mprisFrame->sizeHint().height());
    ui->mprisFrame->raise();

    ui->mprisArt->setPixmap(QIcon::fromTheme("audio-generic").pixmap(32, 32));

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
    delete tscheckpass;

    QTimer* mprisTimer = new QTimer();
    mprisTimer->setInterval(100);
    connect(mprisTimer, SIGNAL(timeout()), this, SLOT(mprisCheckTimer()));
    mprisTimer->start();
    mprisCheckTimer();
}

MainWindow::~MainWindow()
{
    delete ui;
    XUngrabKeyboard(QX11Info::display(), CurrentTime);
    XUngrabPointer(QX11Info::display(), CurrentTime);
}

void MainWindow::mprisCheckTimer() {
    mprisDetectedApps.clear();

    for (QString service : QDBusConnection::sessionBus().interface()->registeredServiceNames().value()) {
        if (service.startsWith("org.mpris.MediaPlayer2.")) {
            if (!mprisDetectedApps.contains(service)) {
                QDBusConnection::sessionBus().connect(service, "/org/mpris/MediaPlayer2/", "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(updateMpris()));
                mprisDetectedApps.append(service);
                if (mprisCurrentAppName == "") {
                    mprisCurrentAppName = service;
                }
                updateMpris();
            }
        }
    }

    if (mprisCurrentAppName != "") {
        ui->mprisFrame->setVisible(true);
        if (!mprisDetectedApps.contains(mprisCurrentAppName)) { //Service closed.
            if (mprisDetectedApps.count() > 0) { //Set to next app
                mprisCurrentAppName = mprisDetectedApps.first();
                ui->mprisFrame->setVisible(true);
                this->resizeSlot();
            } else { //Set to no app. Make mpris controller invisible.
                mprisCurrentAppName = "";
                ui->mprisFrame->setVisible(false);
                this->resizeSlot();
            }
        }
    } else { //Make mpris controller invisible
        ui->mprisFrame->setVisible(false);
    }
}

void MainWindow::updateMpris() {
    if (ui->mprisFrame->isVisible()) {
        if (!pauseMprisMenuUpdate) {
            if (mprisDetectedApps.count() > 1) {
                QMenu* menu = ui->mprisSelection->menu();
                menu->clear();
                for (QString app : mprisDetectedApps) {
                    QAction* action = new QAction();
                    action->setData(app);
                    action->setCheckable(true);
                    if (mprisCurrentAppName == app) {
                        action->setChecked(true);
                    }
                    action->setText(app.remove("org.mpris.MediaPlayer2."));
                    menu->addAction(action);
                }
                //ui->mprisSelection->setMenu(menu);
                ui->mprisSelection->setVisible(true);
            } else {
                ui->mprisSelection->setVisible(false);
            }
        }

        //Get Current Song Metadata
        QDBusMessage MetadataRequest = QDBusMessage::createMethodCall(mprisCurrentAppName, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
        MetadataRequest.setArguments(QList<QVariant>() << "org.mpris.MediaPlayer2.Player" << "Metadata");

        QDBusReply<QDBusVariant> reply(QDBusConnection::sessionBus().call(MetadataRequest));
        QVariantMap replyData;
        QDBusArgument arg(reply.value().variant().value<QDBusArgument>());

        arg >> replyData;

        QString album = "";
        QString artist = "";

        if (replyData.contains("xesam:title")) {
            ui->mprisSongName->setText(replyData.value("xesam:title").toString());
        } else {
            QDBusMessage IdentityRequest = QDBusMessage::createMethodCall(mprisCurrentAppName, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
            IdentityRequest.setArguments(QList<QVariant>() << "org.mpris.MediaPlayer2" << "Identity");

            QDBusReply<QDBusVariant> reply(QDBusConnection::sessionBus().call(IdentityRequest));
            ui->mprisSongName->setText(reply.value().variant().toString());
        }

        if (replyData.contains("xesam:artist")) {
            QStringList artists = replyData.value("xesam:artist").toStringList();
            for (QString art : artists) {
                artist.append(art + ", ");
            }
            artist.remove(artist.length() - 2, 2);
        }

        if (replyData.contains("xesam:album")) {
            album = replyData.value("xesam:album").toString();
        }

        if (artist == "" && album == "") {
            ui->mprisSongArtist->setVisible(false);
        } else if (artist != "" && album == ""){
            ui->mprisSongArtist->setVisible(true);
            ui->mprisSongArtist->setText(artist);
        } else if (artist == "" && album != ""){
            ui->mprisSongArtist->setVisible(true);
            ui->mprisSongArtist->setText(album);
        } else if (artist != "" && album != ""){
            ui->mprisSongArtist->setVisible(true);
            ui->mprisSongArtist->setText(artist + " · " + album);
        }

        //Get Playback Status
        QDBusMessage PlayStatRequest = QDBusMessage::createMethodCall(mprisCurrentAppName, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
        PlayStatRequest.setArguments(QList<QVariant>() << "org.mpris.MediaPlayer2.Player" << "PlaybackStatus");
        QDBusReply<QVariant> PlayStat = QDBusConnection::sessionBus().call(PlayStatRequest);
        if (PlayStat.value().toString() == "Playing") {
            ui->mprisPause->setIcon(QIcon::fromTheme("media-playback-pause"));
        } else {
            ui->mprisPause->setIcon(QIcon::fromTheme("media-playback-start"));

        }
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    this->resizeSlot();

    ui->notificationScroll->setFixedWidth(this->width() - 400);
    //ui->ImageLabel->setGeometry(0, 0, this->width(), this->height());
}

void MainWindow::resizeSlot() {
    int height;
    if (ui->mprisFrame->isVisible()) {
        height = this->height() - ui->mprisFrame->height();
        ui->paddingFrame->resize(this->width(), ui->mprisFrame->height());
    } else {
        height = this->height();
        ui->paddingFrame->resize(this->width(), 0);
    }
    ui->Cover->setGeometry(0, 0, this->width(), height);

    if (imageSize.width() < imageSize.height()) {
        ui->ImageLabel->resize(this->width(), this->width() / ((float) imageSize.width() / (float) imageSize.height()));
        ui->ImageLabel->move(0, height / 2 - ui->ImageLabel->height() / 2);
    } else {
        ui->ImageLabel->resize(height * ((float) imageSize.width() / (float) imageSize.height()), height);
        ui->ImageLabel->move(this->width() / 2 - ui->ImageLabel->width() / 2, 0);
    }

    ui->mprisFrame->setGeometry(0, this->height() - ui->mprisFrame->height(), this->width(), ui->mprisFrame->sizeHint().height());
    ui->timerFrame->setGeometry(0, this->height(), this->width(), this->height());
    ui->hudFrame->setFixedSize(this->width(), ui->hudFrame->sizeHint().height());
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_MediaTogglePlayPause || event->key() == Qt::Key_MediaPlay ||
            event->key() == Qt::Key_MediaPause || event->key() == Qt::Key_MediaStop) {
        ui->mprisPause->click();
    } else if (event->key() == Qt::Key_MediaNext) {
        ui->mprisNext->click();
    } else if (event->key() == Qt::Key_MediaPrevious) {
        ui->mprisBack->click();
    } else if (event->key() == Qt::Key_VolumeDown) {
        tPropertyAnimation* anim = new tPropertyAnimation(ui->hudFrame, "geometry");
        anim->setStartValue(ui->hudFrame->geometry());
        anim->setEndValue(QRect(0, 0, ui->hudFrame->width(), ui->hudFrame->height()));
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->setDuration(500);
        anim->start();

        if (volumeTimer != NULL) {
            volumeTimer->stop();
            volumeTimer->deleteLater();
        }
        volumeTimer = new QTimer();
        volumeTimer->setInterval(3000);
        connect(volumeTimer, &QTimer::timeout, [=] {
            tPropertyAnimation* anim = new tPropertyAnimation(ui->hudFrame, "geometry");
            anim->setStartValue(ui->hudFrame->geometry());
            anim->setEndValue(QRect(0, -ui->hudFrame->height(), ui->hudFrame->width(), ui->hudFrame->height()));
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->setDuration(500);
            anim->start();
        });
        volumeTimer->start();

        //Get Current Limits
        QProcess* mixer = new QProcess(this);
        mixer->start("amixer");
        mixer->waitForFinished();
        QString output(mixer->readAll());

        bool readLine = false;
        int limit;
        QString percent;
        for (QString line : output.split("\n")) {
            if (line.startsWith(" ") && readLine) {
                if (line.startsWith("  Limits:")) {
                    limit = line.split(" ").last().toInt();
                } else if (line.startsWith("  Front Left:")) {
                    if (line.contains("[off]")) {
                        percent = "0";
                    } else {
                        percent = line.mid(line.indexOf("\[") + 1, 3).remove("\%").remove("]");
                    }
                }
            } else {
                if (line.contains("'Master'")) {
                    readLine = true;
                } else {
                    readLine = false;
                }
            }
        }

        int volume = percent.toInt() - 5;

        mixer->start("amixer set Master " + QString::number(limit * (volume / (float) 100)) + " on");
        connect(mixer, SIGNAL(finished(int)), mixer, SLOT(deleteLater()));

        ui->hudLabel->setText("Volume: " + QString::number(volume) + "%");
    } else if (event->key() == Qt::Key_VolumeUp) {
        tPropertyAnimation* anim = new tPropertyAnimation(ui->hudFrame, "geometry");
        anim->setStartValue(ui->hudFrame->geometry());
        anim->setEndValue(QRect(0, 0, ui->hudFrame->width(), ui->hudFrame->height()));
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->setDuration(500);
        anim->start();

        if (volumeTimer != NULL) {
            volumeTimer->stop();
            volumeTimer->deleteLater();
        }
        volumeTimer = new QTimer();
        volumeTimer->setInterval(3000);
        connect(volumeTimer, &QTimer::timeout, [=] {
            tPropertyAnimation* anim = new tPropertyAnimation(ui->hudFrame, "geometry");
            anim->setStartValue(ui->hudFrame->geometry());
            anim->setEndValue(QRect(0, -ui->hudFrame->height(), ui->hudFrame->width(), ui->hudFrame->height()));
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->setDuration(500);
            anim->start();
        });
        volumeTimer->start();

        //Get Current Limits
        QProcess* mixer = new QProcess(this);
        mixer->start("amixer");
        mixer->waitForFinished();
        QString output(mixer->readAll());

        bool readLine = false;
        int limit;
        QString percent;
        for (QString line : output.split("\n")) {
            if (line.startsWith(" ") && readLine) {
                if (line.startsWith("  Limits:")) {
                    limit = line.split(" ").last().toInt();
                } else if (line.startsWith("  Front Left:")) {
                    if (line.contains("[off]")) {
                        percent = "0";
                    } else {
                        percent = line.mid(line.indexOf("\[") + 1, 3).remove("\%").remove("]");
                    }
                }
            } else {
                if (line.contains("'Master'")) {
                    readLine = true;
                } else {
                    readLine = false;
                }
            }
        }

        int volume = percent.toInt() + 5;

        mixer->start("amixer set Master " + QString::number(limit * (volume / (float) 100)) + " on");
        connect(mixer, SIGNAL(finished(int)), mixer, SLOT(deleteLater()));

        ui->hudLabel->setText("Volume: " + QString::number(volume) + "%");
    } else {
        hideCover();
    }
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

        if (idToEmit != -1) {
            QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "invokeAction");
            QVariantList args;
            args.append(idToEmit);
            args.append(actionToEmit);
            message.setArguments(args);
            QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
        }

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

            animation->deleteLater();
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
        connect(animation, SIGNAL(finished()), animation, SLOT(deleteLater()));
        typePassword = false;
        ui->lineEdit->setText("");
        ui->lineEdit->setFocus();
    }
}

void MainWindow::on_lineEdit_textEdited(const QString &arg1)
{
    if (!typePassword) {
        if (arg1 == " ") {
            ui->lineEdit->setText("");
        }
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
    actionToEmit = "";
    idToEmit = -1;
    showCover();
}

void MainWindow::on_mprisPause_clicked()
{
    QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(mprisCurrentAppName, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "PlayPause"), QDBus::NoBlock);
    ui->lineEdit->setFocus();
}

void MainWindow::on_mprisBack_clicked()
{
    QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(mprisCurrentAppName, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "Previous"), QDBus::NoBlock);
    ui->lineEdit->setFocus();
}

void MainWindow::on_mprisNext_clicked()
{
    QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(mprisCurrentAppName, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "Next"), QDBus::NoBlock);
    ui->lineEdit->setFocus();
}

void MainWindow::showNotification(QString summary, QString body, uint id, QStringList actions, QVariantMap hints) {
    if (hints.value("x-thesuite-timercomplete", false).toBool()) {
        QPropertyAnimation* anim = new QPropertyAnimation(ui->timerFrame, "geometry");
        anim->setStartValue(ui->timerFrame->geometry());
        anim->setEndValue(QRect(0, 0, this->width(), this->height()));
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->setDuration(500);
        connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
        anim->start();

        closeTimerId = id;
    } else {
        QFrame* frame = new QFrame();
        /*frame->setFrameShape(QFrame::Box);
        frame->setFrameShadow(QFrame::Raised);
        frame->setAutoFillBackground(true);*/
        QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight);
        frame->setLayout(layout);

        QLabel* icon = new QLabel();
        icon->setPixmap(QIcon::fromTheme("dialog-warning").pixmap(24, 24));
        layout->addWidget(icon);

        QLabel* summaryLab = new QLabel();
        QFont bold = summaryLab->font();
        bold.setBold(true);
        summaryLab->setFont(bold);
        summaryLab->setText(summary);
        layout->addWidget(summaryLab);

        QLabel* bodyLab = new QLabel();
        bodyLab->setText(body);
        bodyLab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        //bodyLab->setWordWrap(true);
        layout->addWidget(bodyLab);

        if (actions.length() == 2) {
            QString action = actions.at(0);
            QPushButton* actionButton = new QPushButton;
            actionButton->setText(actions.at(1));
            connect(actionButton, &QPushButton::clicked, [=]() {
                actionToEmit = action;
                idToEmit = id;
                hideCover();
            });
            layout->addWidget(actionButton);
        } else if (actions.length() != 0) {
            QMenu* menu = new QMenu();
            for (int i = 0; i < actions.length(); i = i + 2) {
                QString action = actions.at(i);
                QString readable = actions.at(i + 1);
                QAction* act = new QAction();
                act->setText(readable);
                act->setData(action);
                connect(act, &QAction::triggered, [=]() {
                    actionToEmit = action;
                    idToEmit = id;
                    hideCover();
                });
                menu->addAction(act);
            }
            QPushButton* actionButton = new QPushButton;
            actionButton->setText("Actions");
            actionButton->setMenu(menu);
            layout->addWidget(actionButton);
        }

        QPushButton* closeButton = new QPushButton();
        closeButton->setIcon(QIcon::fromTheme("window-close"));
        closeButton->setProperty("nId", id);
        closeButton->setFlat(true);
        connect(closeButton, &QPushButton::clicked, [=]() {
            QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "CloseNotification");
            QVariantList args;
            args.append(closeButton->property("nId").toUInt());
            message.setArguments(args);
            QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
        });
        layout->addWidget(closeButton);

        ((QBoxLayout*) ui->scrollAreaWidgetContents->layout())->insertWidget(0, frame);
        notificationHeight += frame->height() + ui->scrollAreaWidgetContents->layout()->spacing();
        notificationFrames.insert(id, frame);

        if (notificationHeight > 250) {
            notificationHeight = 250;
        }

        ui->notificationScroll->resize(ui->notificationScroll->width(), notificationHeight);
        frame->setFixedHeight(frame->sizeHint().height());
        ui->notificationScroll->setVisible(true);
        ui->ImageLabel->setPixmap(QPixmap::fromImage(darkImage));
    }
}

void MainWindow::closeNotification(uint id, uint reason) {
    if (reason != 1) {
        if (closeTimerId == id) {
            QPropertyAnimation* anim = new QPropertyAnimation(ui->timerFrame, "geometry");
            anim->setStartValue(ui->timerFrame->geometry());
            anim->setEndValue(QRect(0, this->height(), this->width(), this->height()));
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->setDuration(500);
            connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
            anim->start();
        } else {
            if (notificationFrames.keys().contains(id)) {
                delete notificationFrames.value(id);
                notificationFrames.remove(id);
            }

            if (notificationFrames.count() == 0) {
                ui->notificationScroll->setVisible(false);
                ui->ImageLabel->setPixmap(QPixmap::fromImage(image));
            }
        }
    }
}

void MainWindow::on_switchUser_clicked()
{
    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.DisplayManager", QString(qgetenv("XDG_SEAT_PATH")), "org.freedesktop.DisplayManager.Seat", "SwitchToGreeter");
    QDBusConnection::systemBus().send(message);
    showCover();
}

void MainWindow::on_mprisSelection_triggered(QAction *arg1)
{
    mprisCurrentAppName = arg1->data().toString();
}

void MainWindow::on_stopTimerButton_clicked()
{
    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "CloseNotification");
    QVariantList args;
    args.append(closeTimerId);
    message.setArguments(args);
    QDBusConnection::sessionBus().call(message, QDBus::NoBlock);

    QPropertyAnimation* anim = new QPropertyAnimation(ui->timerFrame, "geometry");
    anim->setStartValue(ui->timerFrame->geometry());
    anim->setEndValue(QRect(0, this->height(), this->width(), this->height()));
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setDuration(500);
    connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
    anim->start();
}

void MainWindow::animateClose() {
    {
        QRegion reg(0, 0, this->width(), this->height());
        reg = reg.subtracted(QRect(ui->pushButton->mapTo(this, QPoint(0, 0)), ui->pushButton->geometry().size()));
        this->setMask(reg);

        tVariantAnimation* horiz = new tVariantAnimation(this);
        horiz->setStartValue((float) 0);
        horiz->setEndValue((float) 1);
        horiz->setDuration(250);
        horiz->setEasingCurve(QEasingCurve::OutCubic);
        connect(horiz, &tVariantAnimation::valueChanged, [=](QVariant percentage) {
            QRegion reg(0, 0, this->width(), this->height());
            QRect subtraction;
            subtraction.setTop(ui->pushButton->mapTo(this, QPoint(0, 0)).y());
            subtraction.setHeight(ui->pushButton->height());
            int leftButton = ui->pushButton->mapTo(this, QPoint(0, 0)).x();
            int middleButton = leftButton + ui->pushButton->width() / 2;

            //Calculate left area
            int leftArea = (float) middleButton * percentage.toFloat();

            //Calculate right area
            int rightArea = (float) (this->width() - middleButton) * percentage.toFloat();

            subtraction.setLeft(middleButton - leftArea);
            subtraction.setRight(middleButton + rightArea);

            reg = reg.subtracted(subtraction);
            this->setMask(reg);
        });
        connect(horiz, &tVariantAnimation::finished, [=] {
            horiz->deleteLater();

            tVariantAnimation* vert = new tVariantAnimation(this);
            vert->setStartValue((float) 0);
            vert->setEndValue((float) 1);
            vert->setDuration(250);
            vert->setEasingCurve(QEasingCurve::InCubic);
            connect(vert, &tVariantAnimation::valueChanged, [=](QVariant percentage) {
                QRegion reg(0, 0, this->width(), this->height());
                QRect subtraction;
                subtraction.setLeft(0);
                subtraction.setWidth(this->width());

                int topButton = ui->pushButton->mapTo(this, QPoint(0, 0)).y();

                //Calculate top area
                int topArea = (float) topButton * percentage.toFloat();

                //Calculate bottom area
                int bottomArea = (float) (this->height() - (topButton + ui->pushButton->height())) * percentage.toFloat();

                subtraction.setTop(topButton - topArea);
                subtraction.setBottom(topButton + ui->pushButton->height() + bottomArea);

                reg = reg.subtracted(subtraction);
                this->setMask(reg);
            });
            connect(vert, &tVariantAnimation::finished, [=] {
                this->close();
            });
            vert->start();
        });
        horiz->start();
    }
}

void MainWindow::showFullScreen() {
    //this->setWindowFlags(Qt::Window | Qt::X11BypassWindowManagerHint);

    this->setMask(QRegion(0, 0, 1, 1));
    QMainWindow::showFullScreen();
    int end = qSqrt((this->width() / 2) * (this->width() / 2) + (this->height() / 2) * (this->height() / 2));

    tVariantAnimation* radius = new tVariantAnimation(this);
    radius->setStartValue(0);
    radius->setEndValue(end);
    radius->setDuration(200);
    radius->setEasingCurve(QEasingCurve::InCubic);
    connect(radius, &tVariantAnimation::valueChanged, [=](QVariant rad) {
        QRect circle;
        QPoint center(this->width() / 2, this->height() / 2);
        circle.setLeft(center.x() - rad.toInt());
        circle.setTop(center.y() - rad.toInt());
        circle.setWidth(rad.toInt() * 2);
        circle.setHeight(rad.toInt() * 2);

        QRegion mask(circle, QRegion::Ellipse);
        this->setMask(mask);
    });
    connect(radius, SIGNAL(finished()), radius, SLOT(deleteLater()));
    /*QTimer::singleShot(1000, radius, [=] {
        radius->start();
    });*/
    radius->start();
}
