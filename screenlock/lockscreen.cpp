#include "ui_lockscreen.h"
#include "lockscreen.h"

LockScreen::LockScreen(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LockScreen)
{
    ui->setupUi(this);

    background = settings.value("background", "/usr/share/tsscreenlock/triangles.svg").toString();

    QFileInfo backgroundInfo(background);
    if (backgroundInfo.suffix() == "svg") {
        image = QPixmap(this->size());
        QSvgRenderer renderer(background);
        QPainter imagePainter(&image);
        renderer.render(&imagePainter, image.rect());
        imagePainter.fillRect(0, 0, image.width(), image.height(), QColor::fromRgb(0, 0, 0, 127));
    } else {
        image = QPixmap(background);

        QPainter imagePainter(&image);
        imagePainter.fillRect(0, 0, image.width(), image.height(), QColor::fromRgb(0, 0, 0, 127));
    }

    ui->coverArrowUp->setPixmap(QIcon::fromTheme("go-up").pixmap(16, 16));
    ui->availableUsersList->setItemDelegate(new SwitchUserListDelegate());
    ui->mousePasswordErrorLabel->setVisible(false);

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
        ui->usernameLabel->setText(tr("Hello %1!").arg(name));
        ui->usernameLabel_2->setText(tr("Hello %1!").arg(name));
    } else {
        ui->usernameLabel->setText(tr("Hello %1!").arg(fullname));
        ui->usernameLabel_2->setText(tr("Hello %1!").arg(fullname));
    }

    QProcess* tscheckpass = new QProcess();
    tscheckpass->start("tscheckpass " + name);
    tscheckpass->waitForFinished();
    if (tscheckpass->exitCode() == 0) {
        ui->unlockPromptExplanation->setText(tr("Resume your session, continuing where you left off"));
    } else {
        ui->unlockPromptExplanation->setText(tr("Enter your password and resume your session, continuing where you left off"));
    }

    QTimer* timer = new QTimer();
    timer->setInterval(1000);
    connect(timer, SIGNAL(timeout()), this, SLOT(tick()));
    connect(timer, SIGNAL(timeout()), this, SLOT(BatteryChanged()));
    connect(timer, SIGNAL(timeout()), this, SLOT(mprisCheckTimer()));
    timer->start();

    tick();
    BatteriesChanged();
    mprisCheckTimer();

    QDBusConnection::systemBus().connect("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "DeviceAdded", this, SLOT(BatteriesChanged()));
    QDBusConnection::systemBus().connect("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", "DeviceRemoved", this, SLOT(BatteriesChanged()));
    QDBusConnection::sessionBus().connect("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "NotificationClosed", this, SLOT(closeNotification(uint,uint)));

    ui->capsLockOn->setPixmap(QIcon::fromTheme("input-caps-on").pixmap(16, 16));
    ui->password->installEventFilter(this);
    ui->mprisIcon->setPixmap(QIcon::fromTheme("audio").pixmap(32, 32));
    this->setWindowOpacity(0);

    unsigned int capsState;
    XkbGetIndicatorState(QX11Info::display(), XkbUseCoreKbd, &capsState);
    if ((capsState & 0x01) != 1) {
        ui->capsLockOn->setVisible(false);
    }

    passwordFrameOpacity = new QGraphicsOpacityEffect();
    passwordFrameOpacity->setOpacity(0);
    ui->passwordFrame->setGraphicsEffect(passwordFrameOpacity);

    ui->MousePasswordPage->installEventFilter(this);
}

LockScreen::~LockScreen()
{
    delete ui;
}

void LockScreen::showFullScreen() {
    QDialog::showFullScreen();

    this->layout()->removeWidget(ui->coverFrame);
    ui->coverFrame->setParent(this);
    ui->coverFrame->setGeometry(0, 0, this->width(), this->height());
    ui->coverFrame->installEventFilter(this);

    ui->coverFrame->layout()->removeWidget(ui->unlockPromptFrame);
    ui->unlockPromptFrame->setParent(ui->coverFrame);
    ui->unlockPromptFrame->setGeometry(0, this->height(), this->width(), ui->unlockPromptFrame->height());

    this->layout()->removeWidget(ui->passwordFrame);
    ui->passwordFrame->setParent(this);
    ui->passwordFrame->setGeometry(0, 0, this->width(), this->height());

    ui->scrollAreaWidgetContents->installEventFilter(this);

    QTimer::singleShot(1000, [=] {
        tPropertyAnimation* anim = new tPropertyAnimation(ui->unlockPromptFrame, "geometry");
        anim->setStartValue(ui->unlockPromptFrame->geometry());
        anim->setEndValue(QRect(0, this->height() - ui->unlockPromptFrame->height(), this->width(), ui->unlockPromptFrame->height()));
        anim->setDuration(500);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
        anim->start();
    });

    tVariantAnimation* anim = new tVariantAnimation();
    anim->setStartValue((float) 0);
    anim->setEndValue((float) 1);
    anim->setDuration(500);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, &tVariantAnimation::valueChanged, [=](QVariant value) {
        this->setWindowOpacity(value.toFloat());
    });
    connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
    anim->start();

    QMargins currentMargins = ui->coverFrame->contentsMargins();
    currentMargins.setBottom(ui->unlockPromptFrame->height());
    ui->coverFrame->layout()->setContentsMargins(currentMargins);

    TimerNotificationDialog = new TimerComplete(this->geometry());
    connect(TimerNotificationDialog, &TimerComplete::finished, [=] {
        QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "CloseNotification");
        QVariantList args;
        args.append(closeTimerId);
        message.setArguments(args);
        QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
    });

    int nonPasswordLoginTypes = 0;
    QFile mousePasswordFile(QDir::homePath() + "/.theshell/mousepassword");
    if (mousePasswordFile.exists()) {
        //Enable Mouse Password pane
        ui->mousePasswordButton->setVisible(true);
        nonPasswordLoginTypes++;

        mousePasswordFile.open(QFile::ReadOnly);
        mousePassword = mousePasswordFile.readAll().trimmed();

        //Make sure a mouse is connected
        int devicesCount;
        XDeviceInfo* deviceList = XListInputDevices(QX11Info::display(), &devicesCount);

        bool mouseFound;
        for (int i = 0; i < devicesCount; i++) {
            XDeviceInfo device = deviceList[i];
            if (device.type == XInternAtom(QX11Info::display(), XI_MOUSE, True) || device.type == XInternAtom(QX11Info::display(), XI_TRACKBALL, True)) {
                mouseFound = true;
            }
        }
        XFreeDeviceList(deviceList);

        if (mouseFound) {
            //Go to Mouse Password frame
            ui->loginStack->setCurrentIndex(1);
            ui->mousePasswordButton->setChecked(true);
        }
    } else {
        //Don't allow the user to go to the Mouse Password frame
        ui->mousePasswordButton->setVisible(false);
    }

    if (nonPasswordLoginTypes == 0) {
        //Hide Password button since it is the only login method
        ui->passwordButton->setVisible(false);
    }

    ui->password->setFocus();
}

void LockScreen::tick() {
    ui->coverDate->setText(QLocale().toString(QDateTime::currentDateTime().date(), "ddd dd MMM yyyy"));
    ui->coverClock->setText(QLocale().toString(QDateTime::currentDateTime().time(), "hh:mm:ss"));
}

bool LockScreen::eventFilter(QObject *obj, QEvent *e) {
    if (obj == ui->coverFrame) {
        if (e->type() == QEvent::Paint) {
            QPainter painter(ui->coverFrame);

            //Determine rectangle in which to draw image
            QSize paintSize(image.width(), image.height());
            QSize screenSize = this->size();

            QSize fitSize = paintSize.scaled(screenSize, Qt::KeepAspectRatioByExpanding);

            QRect paintRect;
            paintRect.setSize(fitSize);
            paintRect.moveCenter(QPoint(screenSize.width() / 2, screenSize.height() / 2));

            painter.drawPixmap(paintRect, image);

            //Draw cover rectangle
            /*QRect coverRect;
            coverRect.setWidth(this->width());
            coverRect.moveTop(ui->unlockPromptFrame->geometry().bottom());
            coverRect.setHeight(this->height() - ui->unlockPromptFrame->geometry().bottom());
            coverRect.moveLeft(0);

            painter.setPen(Qt::transparent);
            painter.setBrush(Qt::transparent);//this->palette().color(QPalette::Window));
            painter.drawRect(coverRect);*/

            //Draw shade
            float percentShade = 1 - (float) ui->coverFrame->geometry().bottom() / (float) this->height();
            painter.fillRect(paintRect, QColor::fromRgb(0, 0, 0, 127 * percentShade));
            return true;
        } else if (e->type() == QEvent::MouseButtonPress) {
            QMouseEvent* event = (QMouseEvent*) e;
            this->moveY = event->y();
        } else if (e->type() == QEvent::MouseButtonRelease) {
            hideCover();
        } else if (e->type() == QEvent::MouseMove) {
            QMouseEvent* event = (QMouseEvent*) e;

            QRect geometry = ui->coverFrame->geometry();
            //QRect geometry = ui->unlockPromptFrame->geometry();
            geometry.translate(0, 0 - this->moveY + event->y());
            if (geometry.bottom() > this->height()) {
                geometry.moveTop(0);
            }
            geometry.setTop(0);
            ui->coverFrame->setGeometry(geometry);
            //ui->unlockPromptFrame->setGeometry(geometry);
            this->moveY = event->y();

            ui->coverFrame->repaint();
        } else if (e->type() == QEvent::Resize) {
            ui->unlockPromptFrame->setGeometry(0, ui->coverFrame->height() - ui->unlockPromptFrame->height(), this->width(), ui->unlockPromptFrame->height());
        }
    } else if (obj == ui->scrollAreaWidgetContents) {
        if (e->type() == QEvent::Paint) {
            //return true;
        }
    } else if (obj == ui->password) {
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent* event = (QKeyEvent*) e;
            if (event->key() == Qt::Key_Return && event->modifiers() == Qt::MetaModifier) {
                this->keyPressEvent(event);
                return true;
            }
            if (!coverHidden) {
                hideCover();
                return true;
            }
        } else if (e->type() == QEvent::KeyRelease) {
            unsigned int capsState;
            XkbGetIndicatorState(QX11Info::display(), XkbUseCoreKbd, &capsState);
            if ((capsState & 0x01) == 1) {
                ui->capsLockOn->setVisible(true);
            } else {
                ui->capsLockOn->setVisible(false);
            }
        }
    } else if (obj == ui->MousePasswordPage) {
        if ((e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonDblClick) && mousePasswordWrongCount < 5) {
            QMouseEvent* event = (QMouseEvent*) e;

            switch (event->button()) {
                case Qt::LeftButton:
                    currentMousePassword += 'q';
                    break;
                case Qt::MiddleButton:
                    currentMousePassword += 'w';
                    break;
                case Qt::RightButton:
                    currentMousePassword += 'e';
                    break;
                case Qt::ExtraButton1:
                    currentMousePassword += 'r';
                    break;
                case Qt::ExtraButton2:
                    currentMousePassword += 't';
                    break;
                case Qt::ExtraButton3:
                    currentMousePassword += 'y';
                    break;
                case Qt::ExtraButton4:
                    currentMousePassword += 'u';
                    break;
                case Qt::ExtraButton5:
                    currentMousePassword += 'i';
                    break;
                case Qt::ExtraButton6:
                    currentMousePassword += 'o';
                    break;
            }

            if (currentMousePassword.length() > 100) currentMousePassword = currentMousePassword.left(100);
            checkMousePassword();
            return true;
        } else if (e->type() == QEvent::MouseButtonRelease && mousePasswordWrongCount < 5) {
            QMouseEvent* event = (QMouseEvent*) e;
            switch (event->button()) {
                case Qt::LeftButton:
                    currentMousePassword += 'a';
                    break;
                case Qt::MiddleButton:
                    currentMousePassword += 's';
                    break;
                case Qt::RightButton:
                    currentMousePassword += 'd';
                    break;
                case Qt::ExtraButton1:
                    currentMousePassword += 'f';
                    break;
                case Qt::ExtraButton2:
                    currentMousePassword += 'g';
                    break;
                case Qt::ExtraButton3:
                    currentMousePassword += 'h';
                    break;
                case Qt::ExtraButton4:
                    currentMousePassword += 'j';
                    break;
                case Qt::ExtraButton5:
                    currentMousePassword += 'k';
                    break;
                case Qt::ExtraButton6:
                    currentMousePassword += 'l';
                    break;
            }

            if (currentMousePassword.length() > 100) currentMousePassword = currentMousePassword.left(100);
            checkMousePassword();
            return true;
        }
    }
    return false;
}

void LockScreen::checkMousePassword() {
    if (mousePasswordWrongCount >= 5) {
        ui->mousePasswordActiveFrame->setVisible(false);
        ui->mousePasswordPrompt->setText(tr("Mouse Password is disabled"));
        ui->mousePasswordErrorLabel->setVisible(true);
    } else {
        ui->mousePasswordEchoLabel->setText(QString(currentMousePassword.count(), QChar(0x2022 /* Bullet character */)));
        if (strcmp(crypt(currentMousePassword.data(), mousePassword.toLocal8Bit().data()), mousePassword.toLocal8Bit().data()) == 0) {
            //Allow access
            XUngrabKeyboard(QX11Info::display(), CurrentTime);
            XUngrabPointer(QX11Info::display(), CurrentTime);

            QApplication::exit();
        }
    }
}

void LockScreen::resizeEvent(QResizeEvent *event) {
    QFileInfo backgroundInfo(background);
    ui->coverFrame->setGeometry(0, 0, this->width(), this->height());
    ui->passwordFrame->setGeometry(0, 0, this->width(), this->height());
    if (backgroundInfo.suffix() == "svg") {
        image = QPixmap(this->size());
        QSvgRenderer renderer(background);
        QPainter imagePainter(&image);
        renderer.render(&imagePainter, image.rect());
        imagePainter.fillRect(0, 0, image.width(), image.height(), QColor::fromRgb(0, 0, 0, 127));
    }
}



void LockScreen::BatteriesChanged() {
    allDevices.clear();
    QDBusInterface *i = new QDBusInterface("org.freedesktop.UPower", "/org/freedesktop/UPower", "org.freedesktop.UPower", QDBusConnection::systemBus(), this);
    QDBusReply<QList<QDBusObjectPath>> reply = i->call("EnumerateDevices"); //Get all devices

    if (reply.isValid()) { //Check if the reply is ok
        for (QDBusObjectPath device : reply.value()) {
            if (device.path().contains("battery") || device.path().contains("media_player") || device.path().contains("computer") || device.path().contains("phone")) { //This is a battery or media player or tablet computer
                QDBusConnection::systemBus().connect("org.freedesktop.UPower", device.path(),
                                                     "org.freedesktop.DBus.Properties", "PropertiesChanged", this,
                                                                      SLOT(DeviceChanged()));

                QDBusInterface *i = new QDBusInterface("org.freedesktop.UPower", device.path(), "org.freedesktop.UPower.Device", QDBusConnection::systemBus(), this);
                allDevices.append(i);
            }
        }
        BatteryChanged();
    } else {
        ui->batteryFrame->setVisible(true);
        ui->BatteryLabel->setText("Can't get battery information.");
    }
}


void LockScreen::BatteryChanged() {
    QStringList displayOutput;

    int batLevel;
    bool hasPCBat = false;
    bool isCharging;
    for (QDBusInterface *i : allDevices) {
        //Get the percentage of battery remaining.
        //We do the calculation ourselves because UPower can be inaccurate sometimes
        double percentage;

        //Check that the battery actually reports energy information
        double energyFull = i->property("EnergyFull").toDouble();
        double energy = i->property("Energy").toDouble();
        double energyEmpty = i->property("EnergyEmpty").toDouble();
        if (energyFull == 0 && energy == 0 && energyEmpty == 0) {
            //The battery does not report energy information, so get the percentage from UPower.
            percentage = i->property("Percentage").toDouble();
        } else {
            //Calculate the percentage ourself, and round it to an integer.
            //Add 0.5 because C++ always rounds down.
            percentage = (int) (((energy - energyEmpty) / (energyFull - energyEmpty) * 100) + 0.5);
        }
        if (i->path().contains("battery")) {
            //PC Battery
            if (i->property("IsPresent").toBool()) {
                hasPCBat = true;
                bool showRed = false;
                qulonglong timeToFull = i->property("TimeToFull").toULongLong();
                qulonglong timeToEmpty = i->property("TimeToEmpty").toULongLong();
                QDateTime timeRemain;

                //Depending on the state, do different things.
                QString state;
                switch (i->property("State").toUInt()) {
                case 1: //Charging
                    state = " (" + tr("Charging");

                    isCharging = true;

                    if (timeToFull != 0) {
                        timeRemain = QDateTime::fromTime_t(timeToFull).toUTC();
                        state.append(" · " + timeRemain.toString("h:mm"));
                    } else {
                        timeRemain = QDateTime(QDate(0, 0, 0));
                    }

                    state += ")";
                    break;
                case 2: //Discharging
                    //state = " (" + tr("Discharging");
                    state = " (";

                    if (timeToEmpty != 0) {
                        timeRemain = QDateTime::fromTime_t(timeToEmpty).toUTC();
                        state.append(/*" · " + */timeRemain.toString("h:mm"));
                    } else {
                        timeRemain = QDateTime(QDate(0, 0, 0));
                    }
                    state += ")";

                    isCharging = false;
                    break;
                case 3: //Empty
                    state = " (" + tr("Empty") + ")";
                    break;
                case 4: //Charged
                case 6: //Pending Discharge
                    state = " (" + tr("Full") + ")";
                    timeRemain = QDateTime(QDate(0, 0, 0));
                    isCharging = false;
                    break;
                case 5: //Pending Charge
                    state = " (" + tr("Not Charging") + ")";
                    break;
                }

                if (showRed) {
                    displayOutput.append("<span style=\"background-color: red; color: black;\">" + tr("%1% PC Battery%2").arg(QString::number(percentage), state) + "</span>");
                } else {
                    displayOutput.append(tr("%1% PC Battery%2").arg(QString::number(percentage), state));
                }

                batLevel = percentage;
            } else {
                displayOutput.append(tr("No Battery Inserted"));
            }
        } else if (i->path().contains("media_player") || i->path().contains("computer") || i->path().contains("phone")) {
            //This is an external media player (or tablet)
            //Get the model of this media player
            QString model = i->property("Model").toString();

            if (i->property("Serial").toString().length() == 40 && i->property("Vendor").toString().contains("Apple") && QFile("/usr/bin/idevice_id").exists()) { //This is probably an iOS device
                //Get the name of the iOS device
                QProcess iosName;
                iosName.start("idevice_id " + i->property("Serial").toString());
                iosName.waitForFinished();

                QString name(iosName.readAll());
                name = name.trimmed();

                if (name != "" && !name.startsWith("ERROR:")) {
                    model = name;
                }
            }
            if (i->property("State").toUInt() == 0) {
                if (QFile("/usr/bin/thefile").exists()) {
                    displayOutput.append(tr("Pair %1 using theFile to see battery status.").arg(model));
                } else {
                    displayOutput.append(tr("%1 battery unavailable. Device trusted?").arg(model));
                }
            } else {
                QString batteryText;
                batteryText.append(tr("%1% battery on %2").arg(QString::number(percentage), model));
                switch (i->property("State").toUInt()) {
                case 1:
                    batteryText.append(" (" + tr("Charging") + ")");
                    break;
                case 2:
                    batteryText.append(" (" + tr("Discharging") + ")");
                    break;
                case 3:
                    batteryText.append(" (" + tr("Empty") + ")");
                    break;
                case 4:
                case 6:
                    batteryText.append(" (" + tr("Full") + ")");
                    break;
                case 5:
                    batteryText.append(" (" + tr("Not Charging") + ")");
                    break;
                }
                displayOutput.append(batteryText)
                        ;
            }
        }
    }

    //If KDE Connect is running, check the battery status of connected devices.
    if (QDBusConnection::sessionBus().interface()->registeredServiceNames().value().contains("org.kde.kdeconnect")) {
        //Get connected devices
        QDBusMessage devicesMessage = QDBusMessage::createMethodCall("org.kde.kdeconnect", "/modules/kdeconnect", "org.kde.kdeconnect.daemon", "devices");
        devicesMessage.setArguments(QVariantList() << true);
        QDBusReply<QStringList> connectedDevices = QDBusConnection::sessionBus().call(devicesMessage, QDBus::Block, 5000);
        if (connectedDevices.isValid()) {
            for (QString device : connectedDevices.value()) {
                QDBusInterface interface("org.kde.kdeconnect", "/modules/kdeconnect/devices/" + device, "org.kde.kdeconnect.device");
                QString name = interface.property("name").toString();
                QDBusInterface batteryInterface("org.kde.kdeconnect", "/modules/kdeconnect/devices/" + device, "org.kde.kdeconnect.device.battery");
                if (batteryInterface.isValid()) {
                    QDBusReply<int> currentCharge = batteryInterface.call("charge");
                    QDBusReply<bool> charging = batteryInterface.call("isCharging");

                    if (currentCharge.isValid()) {
                        if (currentCharge != -1) {
                            QString batteryText;
                            if (charging) {
                                if (currentCharge == 100) {
                                    batteryText = tr("%1% battery on %2 (Full)").arg(QString::number(currentCharge), name);
                                } else {
                                    batteryText = tr("%1% battery on %2 (Charging)").arg(QString::number(currentCharge), name);
                                }
                            } else {
                                batteryText = tr("%1% battery on %2 (Discharging)").arg(QString::number(currentCharge), name);
                            }
                            displayOutput.append(batteryText);
                        }
                    }
                }
            }
        }
    }

    if (displayOutput.count() == 0) {
        ui->batteryFrame->setVisible(false);
    } else {
        ui->batteryFrame->setVisible(true);
        ui->BatteryLabel->setText(displayOutput.join(" · "));

        QString iconName;
        if (isCharging) {
            if (batLevel < 10) {
                iconName = "battery-charging-empty";
            } else if (batLevel < 30) {
                iconName = "battery-charging-020";
            } else if (batLevel < 50) {
                iconName = "battery-charging-040";
            } else if (batLevel < 70) {
                iconName = "battery-charging-060";
            } else if (batLevel < 90) {
                iconName = "battery-charging-080";
            } else {
                iconName = "battery-charging-100";
            }
        } else if (theLibsGlobal::instance()->powerStretchEnabled()) {
            if (batLevel < 10) {
                iconName = "battery-stretch-empty";
            } else if (batLevel < 30) {
                iconName = "battery-stretch-020";
            } else if (batLevel < 50) {
                iconName = "battery-stretch-040";
            } else if (batLevel < 70) {
                iconName = "battery-stretch-060";
            } else if (batLevel < 90) {
                iconName = "battery-stretch-080";
            } else {
                iconName = "battery-stretch-100";
            }
        } else {
            if (batLevel < 10) {
                iconName = "battery-empty";
            } else if (batLevel < 30) {
                iconName = "battery-020";
            } else if (batLevel < 50) {
                iconName = "battery-040";
            } else if (batLevel < 70) {
                iconName = "battery-060";
            } else if (batLevel < 90) {
                iconName = "battery-080";
            } else {
                iconName = "battery-100";
            }
        }

        ui->BatteryIcon->setPixmap(QIcon::fromTheme(iconName).pixmap(16, 16));
    }
}


void LockScreen::hideCover() {
    //if (!typePassword) {
        tPropertyAnimation* animation = new tPropertyAnimation(ui->coverFrame, "geometry");
        animation->setStartValue(ui->coverFrame->geometry());
        animation->setEndValue(QRect(0, 0, this->width(), 0));
        animation->setDuration(500);
        animation->setEasingCurve(QEasingCurve::OutCubic);
        animation->start();
        connect(animation, &tPropertyAnimation::finished, [=]() {
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

        tPropertyAnimation* opacity = new tPropertyAnimation(passwordFrameOpacity, "opacity");
        opacity->setStartValue((float) 0);
        opacity->setEndValue((float) 1);
        opacity->setDuration(500);
        opacity->setEasingCurve(QEasingCurve::OutCubic);
        connect(opacity, SIGNAL(finished()), opacity, SLOT(deleteLater()));
        opacity->start();

        QTimer::singleShot(100, [=] {
            ui->MouseUnderline->startAnimation();
            ui->PasswordUnderline->startAnimation();
        });

        //typePassword = true;
        coverHidden = true;
        ui->password->setFocus();
    //}
}

void LockScreen::showCover() {
    //if (typePassword) {
        tPropertyAnimation* animation = new tPropertyAnimation(ui->coverFrame, "geometry");
        animation->setStartValue(ui->coverFrame->geometry());
        animation->setEndValue(QRect(0, 0, this->width(), this->height()));
        animation->setDuration(500);
        animation->setEasingCurve(QEasingCurve::OutCubic);
        animation->start();
        connect(animation, SIGNAL(finished()), animation, SLOT(deleteLater()));
        connect(animation, &tPropertyAnimation::finished, [=] {
            passwordFrameOpacity->setOpacity(0);
        });

        tPropertyAnimation* opacity = new tPropertyAnimation(passwordFrameOpacity, "opacity");
        opacity->setStartValue((float) 1);
        opacity->setEndValue((float) 0);
        opacity->setDuration(500);
        opacity->setEasingCurve(QEasingCurve::OutCubic);
        connect(opacity, SIGNAL(finished()), opacity, SLOT(deleteLater()));
        opacity->start();

        //typePassword = false;
        ui->password->setText("");
        ui->password->setFocus();

        ui->unlockPromptFrame->setGeometry(0, this->height() - ui->unlockPromptFrame->height(), this->width(), ui->unlockPromptFrame->height());
        coverHidden = false;
    //}
}

void LockScreen::on_goBackButton_clicked()
{
    showCover();
}

void LockScreen::on_unlockButton_clicked()
{
    ui->password->setEnabled(false);
    ui->unlockButton->setEnabled(false);
    QString name = qgetenv("USER");
    if (name.isEmpty()) {
        name = qgetenv("USERNAME");
    }

    QProcess tscheckpass;
    tscheckpass.start("tscheckpass " + name + " " + ui->password->text());
    tscheckpass.waitForFinished();
    if (tscheckpass.exitCode() == 0) {
        XUngrabKeyboard(QX11Info::display(), CurrentTime);
        XUngrabPointer(QX11Info::display(), CurrentTime);

        /*if (idToEmit != -1) {
            QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "invokeAction");
            QVariantList args;
            args.append(idToEmit);
            args.append(actionToEmit);
            message.setArguments(args);
            QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
        }*/

        QApplication::exit();
    } else {
        QTimer::singleShot(1000, [=] {
            ui->password->setText("");

            tToast* toast = new tToast();
            toast->setTitle(tr("Incorrect Password"));
            toast->setText(tr("The password you entered was incorrect. Please enter your password again."));
            connect(toast, SIGNAL(dismissed()), toast, SLOT(deleteLater()));
            toast->show(this);

            ui->password->setEnabled(true);
            ui->unlockButton->setEnabled(true);
            ui->password->setFocus();
        });
        return;
    }

    ui->password->setEnabled(true);
    ui->unlockButton->setEnabled(true);
}

void LockScreen::on_password_returnPressed()
{
    ui->unlockButton->click();
}

void LockScreen::showNotification(QString summary, QString body, uint id, QStringList actions, QVariantMap hints) {
    if (hints.value("x-thesuite-timercomplete", false).toBool()) {
        TimerNotificationDialog->showFullScreen();

        closeTimerId = id;
    } else {
        QFrame* frame = new QFrame();
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

        ((QBoxLayout*) ui->scrollAreaWidgetContents->layout())->insertWidget(ui->scrollAreaWidgetContents->layout()->count() - 1, frame);
        notificationHeight += frame->height() + ui->scrollAreaWidgetContents->layout()->spacing();
        notificationFrames.insert(id, frame);

        if (notificationHeight > 250) {
            notificationHeight = 250;
        }

        ui->notificationScroll->resize(ui->notificationScroll->width(), notificationHeight);
        frame->setFixedHeight(frame->sizeHint().height());
    }
}

void LockScreen::closeNotification(uint id, uint reason) {
    if (reason != 1) {
        if (closeTimerId == id) {
            TimerNotificationDialog->close();
        } else {
            if (notificationFrames.keys().contains(id)) {
                delete notificationFrames.value(id);
                notificationFrames.remove(id);
            }
        }
    }
}

void LockScreen::on_TurnOffScreenButton_clicked()
{
    showCover();
    QProcess::startDetached("xset dpms force off");
}

void LockScreen::mprisCheckTimer() {
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
        ui->mprisControllerFrame->setVisible(true);
        if (!mprisDetectedApps.contains(mprisCurrentAppName)) { //Service closed.
            if (mprisDetectedApps.count() > 0) { //Set to next app
                mprisCurrentAppName = mprisDetectedApps.first();
                ui->mprisControllerFrame->setVisible(true);
                //this->resizeSlot();
            } else { //Set to no app. Make mpris controller invisible.
                mprisCurrentAppName = "";
                ui->mprisControllerFrame->setVisible(false);
                //this->resizeSlot();
            }
        }
    } else { //Make mpris controller invisible
        ui->mprisControllerFrame->setVisible(false);
    }
}

void LockScreen::updateMpris() {
    if (ui->mprisControllerFrame->isVisible()) {
        if (!pauseMprisMenuUpdate) {
            if (mprisDetectedApps.count() > 1) {
                QMenu* menu = ui->mprisSelection->menu();

                if (menu == NULL) {
                    menu = new QMenu();
                }

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
                ui->mprisSelection->setMenu(menu);
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
            ui->mprisTitle->setText(replyData.value("xesam:title").toString());
        } else {
            QDBusMessage IdentityRequest = QDBusMessage::createMethodCall(mprisCurrentAppName, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
            IdentityRequest.setArguments(QList<QVariant>() << "org.mpris.MediaPlayer2" << "Identity");

            QDBusReply<QDBusVariant> reply(QDBusConnection::sessionBus().call(IdentityRequest));
            ui->mprisTitle->setText(reply.value().variant().toString());
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
            ui->mprisMetadata->setVisible(false);
        } else if (artist != "" && album == ""){
            ui->mprisMetadata->setVisible(true);
            ui->mprisMetadata->setText(artist);
        } else if (artist == "" && album != ""){
            ui->mprisMetadata->setVisible(true);
            ui->mprisMetadata->setText(album);
        } else if (artist != "" && album != ""){
            ui->mprisMetadata->setVisible(true);
            ui->mprisMetadata->setText(artist + " · " + album);
        }

        //Get Playback Status
        QDBusMessage PlayStatRequest = QDBusMessage::createMethodCall(mprisCurrentAppName, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
        PlayStatRequest.setArguments(QList<QVariant>() << "org.mpris.MediaPlayer2.Player" << "PlaybackStatus");
        QDBusReply<QVariant> PlayStat = QDBusConnection::sessionBus().call(PlayStatRequest);
        if (PlayStat.value().toString() == "Playing") {
            ui->mprisPlay->setIcon(QIcon::fromTheme("media-playback-pause"));
        } else {
            ui->mprisPlay->setIcon(QIcon::fromTheme("media-playback-start"));
        }
    }
}

void LockScreen::on_mprisPlay_clicked()
{
    QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(mprisCurrentAppName, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "PlayPause"), QDBus::NoBlock);
    ui->password->setFocus();
}

void LockScreen::on_mprisNext_clicked()
{
    QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(mprisCurrentAppName, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "Next"), QDBus::NoBlock);
    ui->password->setFocus();
}

void LockScreen::on_mprisBack_clicked()
{
    QDBusConnection::sessionBus().call(QDBusMessage::createMethodCall(mprisCurrentAppName, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", "Previous"), QDBus::NoBlock);
    ui->password->setFocus();
}

void LockScreen::animateClose() {
    tPropertyAnimation* passUp = new tPropertyAnimation(ui->passwordFrame, "geometry");
    passUp->setStartValue(ui->passwordFrame->geometry());
    passUp->setEndValue(QRect(0, -200, this->width(), this->height()));
    passUp->setDuration(250);
    passUp->setEasingCurve(QEasingCurve::InCubic);
    connect(passUp, SIGNAL(finished()), passUp, SLOT(deleteLater()));

    tPropertyAnimation* opacity = new tPropertyAnimation(passwordFrameOpacity, "opacity");
    opacity->setStartValue((float) 1);
    opacity->setEndValue((float) 0);
    opacity->setDuration(250);
    opacity->setEasingCurve(QEasingCurve::InCubic);
    connect(opacity, SIGNAL(finished()), opacity, SLOT(deleteLater()));
    connect(opacity, &tPropertyAnimation::finished, [=] {
        tVariantAnimation* anim = new tVariantAnimation();
        anim->setStartValue((float) 1);
        anim->setEndValue((float) 0);
        anim->setDuration(250);
        anim->setEasingCurve(QEasingCurve::InCubic);
        connect(anim, &tVariantAnimation::valueChanged, [=](QVariant value) {
            this->setWindowOpacity(value.toFloat());
        });
        connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
        connect(anim, &tVariantAnimation::finished, [=] {
            this->close();
        });

        anim->start();
    });
    opacity->start();
    passUp->start();
}

void LockScreen::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_MediaTogglePlayPause || event->key() == Qt::Key_MediaPlay ||
            event->key() == Qt::Key_MediaPause || event->key() == Qt::Key_MediaStop) {
        ui->mprisPlay->click();
    } else if (event->key() == Qt::Key_MediaNext) {
        ui->mprisNext->click();
    } else if (event->key() == Qt::Key_MediaPrevious) {
        ui->mprisBack->click();
    } else if (event->key() == Qt::Key_Return && event->modifiers() == Qt::MetaModifier) {
        QDBusMessage m = QDBusMessage::createMethodCall("org.thesuite.theshell", "/org/thesuite/theshell", "org.thesuite.theshell", "NextKeyboard");
        QDBusConnection::sessionBus().call(m, QDBus::NoBlock);
    }
}

void LockScreen::on_SuspendButton_clicked()
{
    showCover();
    QList<QVariant> arguments;
    arguments.append(true);

    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", "Suspend");
    message.setArguments(arguments);
    QDBusConnection::systemBus().send(message);
}

struct LoginSession {
    QString sessionId;
    uint userId;
    QString username;
    QString seat;
    QDBusObjectPath path;
};
Q_DECLARE_METATYPE(LoginSession)

const QDBusArgument &operator<<(QDBusArgument &argument, const LoginSession &session) {
    argument.beginStructure();
    argument << session.sessionId << session.userId << session.username << session.seat << session.path;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, LoginSession &session) {
    argument.beginStructure();
    argument >> session.sessionId >> session.userId >> session.username >> session.seat >> session.path;
    argument.endStructure();
    return argument;
}

void LockScreen::on_switchUserButton_clicked()
{
    //Load available users
    QDBusInterface i("org.freedesktop.login1", "/org/freedesktop/login1/session/self", "org.freedesktop.login1.Session", QDBusConnection::systemBus());
    QString thisId = i.property("Id").toString();

    QDBusMessage availableSessions = QDBusMessage::createMethodCall("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", "ListSessions");
    QDBusMessage availableSessionsReply = QDBusConnection::systemBus().call(availableSessions);

    QDBusArgument availableSessionsArg = availableSessionsReply.arguments().first().value<QDBusArgument>();
    QList<LoginSession> sessions;
    availableSessionsArg >> sessions;

    ui->availableUsersList->clear();
    for (LoginSession session : sessions) {
        if (session.sessionId != thisId) {
            QListWidgetItem* item = new QListWidgetItem();

            QDBusInterface i("org.freedesktop.login1", session.path.path(), "org.freedesktop.login1.Session", QDBusConnection::systemBus());
            QString type = i.property("Type").toString();

            QString cls = i.property("Class").toString();
            if (cls == "user") {
                QDBusMessage accountsMessage = QDBusMessage::createMethodCall("org.freedesktop.Accounts", "/org/freedesktop/Accounts", "org.freedesktop.Accounts", "FindUserById");
                accountsMessage.setArguments(QList<QVariant>() << (qlonglong) session.userId);
                QDBusMessage accountsMessageReply = QDBusConnection::systemBus().call(accountsMessage);

                QDBusObjectPath accountObjectPath = accountsMessageReply.arguments().first().value<QDBusObjectPath>();
                QDBusInterface userInterface("org.freedesktop.Accounts", accountObjectPath.path(), "org.freedesktop.Accounts.User", QDBusConnection::systemBus());

                QString name = userInterface.property("RealName").toString();
                if (name == "") {
                    name = session.username;
                }
                item->setText(name);
                item->setIcon(QIcon::fromTheme("user"));

                QString desktop = i.property("Desktop").toString();

                QString secondLine;
                if (type == "x11") {
                    secondLine = tr("%1 on X11 display %2").arg(desktop, i.property("Display").toString());
                } else if (type == "tty") {
                    secondLine = tr("on %1").arg(i.property("TTY").toString());
                } else if (type == "wayland") {
                    secondLine = tr("%1 on VT #%2").arg(desktop, QString::number(i.property("VTNr").toUInt()));
                } else {
                    secondLine = tr("Session");
                }
                item->setData(Qt::UserRole + 1, secondLine);
            } else if (cls == "greeter") {
                item->setText(session.username);
                item->setIcon(QIcon::fromTheme("arrow-right"));
                item->setData(Qt::UserRole + 1, "Login Screen");
            } else {
                delete item;
                continue;
            }
            item->setData(Qt::UserRole, session.path.path());

            ui->availableUsersList->addItem(item);
        }
    }

    if (ui->availableUsersList->count() == 0) {
        ui->newSessionButton->click();
    } else {
        ui->pagesStack->setCurrentIndex(1);
    }
}

void LockScreen::on_passwordButton_toggled(bool checked)
{
    if (checked) {
        ui->loginStack->setCurrentIndex(0);
        ui->password->setFocus();
    }
}

void LockScreen::on_mousePasswordButton_toggled(bool checked)
{
    if (checked) {
        ui->loginStack->setCurrentIndex(1);
    }
}

void LockScreen::on_loginStack_currentChanged(int arg1)
{
    switch (arg1) {
        case 0:
            ui->passwordButton->setChecked(true);
            break;
        case 1:
            ui->mousePasswordButton->setChecked(true);
            break;
    }
}

void LockScreen::on_retryMousePasswordButton_clicked()
{
    if (currentMousePassword.length() > 5) {
        mousePasswordWrongCount++;
    }
    currentMousePassword.clear();
    checkMousePassword();
}

void LockScreen::on_backToLogin_clicked()
{
    ui->pagesStack->setCurrentIndex(0);
}

void LockScreen::on_newSessionButton_clicked()
{
    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.DisplayManager", QString(qgetenv("XDG_SEAT_PATH")), "org.freedesktop.DisplayManager.Seat", "SwitchToGreeter");
    QDBusConnection::systemBus().send(message);
    ui->pagesStack->setCurrentIndex(0);
    showCover();
}

void LockScreen::on_availableUsersList_itemActivated(QListWidgetItem *item)
{
    ui->pagesStack->setCurrentIndex(0);
    showCover();

    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.login1", item->data(Qt::UserRole).toString(), "org.freedesktop.login1.Session", "Activate");
    QDBusConnection::systemBus().send(message);
}
