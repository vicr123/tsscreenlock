#include "ui_lockscreen.h"
#include "lockscreen.h"

#include <Wm/desktopwm.h>
#include <SystemSlide/systemslide.h>

struct LockScreenPrivate {
    SystemSlide* slide;
    QTimer* initialTimer;
    QTimer* sleepTimer;
};

LockScreen::LockScreen(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::LockScreen) {
    ui->setupUi(this);
    d = new LockScreenPrivate();
    d->slide = new SystemSlide(this);

    connect(d->slide, &SystemSlide::deactivated, [ = ] {
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

        tPropertyAnimation* opacity = new tPropertyAnimation(passwordFrameOpacity, "opacity");
        opacity->setStartValue(0.0);
        opacity->setEndValue(1.0);
        opacity->setDuration(500);
        opacity->setEasingCurve(QEasingCurve::OutCubic);
        connect(opacity, SIGNAL(finished()), opacity, SLOT(deleteLater()));
        opacity->start();

        QTimer::singleShot(100, [ = ] {
            ui->MouseUnderline->startAnimation();
            ui->PasswordUnderline->startAnimation();
        });

        ui->password->setFocus();
    });

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

    QStringList nameParts = parseName.split(",").at(0).split(":");
    QString fullname;
    if (nameParts.count() > 4) {
        fullname = nameParts.at(4);
    }
    if (fullname == "") {
        ui->usernameLabel->setText(tr("Hello %1!").arg(name));
        ui->usernameLabel_2->setText(tr("Hello %1!").arg(name));
    } else {
        ui->usernameLabel->setText(tr("Hello %1!").arg(fullname));
        ui->usernameLabel_2->setText(tr("Hello %1!").arg(fullname));
    }

    QString description;
    QProcess* tscheckpass = new QProcess();
    tscheckpass->start("tscheckpass " + name);
    tscheckpass->waitForFinished();
    if (tscheckpass->exitCode() == 0) {
        description = tr("Resume your session, continuing where you left off");
    } else {
        description = tr("Enter your password and resume your session, continuing where you left off");
        d->slide->setDragResultWidget(ui->page_2);
    }

    d->slide->setBackgroundMode(SystemSlide::DesktopBackground);
    d->slide->setAction(tr("Unlock"), description);
    d->slide->setActionIcon(QIcon::fromTheme("go-up"));

    ui->capsLockOn->setPixmap(QIcon::fromTheme("input-caps-on").pixmap(16, 16));
    ui->password->installEventFilter(this);
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

    //Create an instance of DesktopWm
    DesktopWm::instance();

    d->initialTimer = new QTimer(this);
    d->initialTimer->setInterval(5000);
    d->initialTimer->setSingleShot(true);
    connect(d->initialTimer, &QTimer::timeout, this, [ = ] {
        //If there has been no activity for 4 seconds, turn off the screen
        if (DesktopWm::msecsIdle() >= 4000) DesktopWm::setScreenOff(true);
    });
    d->initialTimer->start();

    d->sleepTimer = new QTimer(this);
    d->sleepTimer->setInterval(5000);
    connect(d->sleepTimer, &QTimer::timeout, this, [ = ] {
        //If the slide is active and there has been no activity for 60 seconds, turn off the screen
        if (!d->slide->isActive()) return;
        if (DesktopWm::isScreenOff()) return;
        if (DesktopWm::msecsIdle() >= 60000) DesktopWm::setScreenOff(true);
    });
    d->sleepTimer->start();
}

LockScreen::~LockScreen() {
    delete ui;
    delete d;
}

void LockScreen::showFullScreen() {
    QDialog::showFullScreen();

    tVariantAnimation* anim = new tVariantAnimation();
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setDuration(500);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, &tVariantAnimation::valueChanged, [ = ](QVariant value) {
        this->setWindowOpacity(value.toReal());
    });
    connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
    anim->start();

    TimerNotificationDialog = new TimerComplete(this->geometry());
    connect(TimerNotificationDialog, &TimerComplete::finished, [ = ] {
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

        bool mouseFound = false;
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

bool LockScreen::eventFilter(QObject* obj, QEvent* e) {
    if (obj == ui->password) {
        if (e->type() == QEvent::KeyPress) {
            QKeyEvent* event = (QKeyEvent*) e;
            if (event->key() == Qt::Key_Return && event->modifiers() == Qt::MetaModifier) {
                this->keyPressEvent(event);
                return true;
            }
            if (d->slide->isActive()) {
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

void LockScreen::hideCover() {
    d->slide->deactivate();
}

void LockScreen::showCover() {
    d->slide->activate();

    ui->password->setText("");
    ui->password->setFocus();
}

void LockScreen::on_goBackButton_clicked() {
    showCover();

    d->initialTimer->stop();
    d->initialTimer->start();
}

void LockScreen::on_unlockButton_clicked() {
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
        QTimer::singleShot(1000, [ = ] {
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

void LockScreen::on_password_returnPressed() {
    ui->unlockButton->click();
}

void LockScreen::showNotification(QString summary, QString body, uint id, QStringList actions, QVariantMap hints) {
    if (hints.value("x-thesuite-timercomplete", false).toBool()) {
        TimerNotificationDialog->showFullScreen();

        closeTimerId = id;
    } else {
        //TODO
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

void LockScreen::on_TurnOffScreenButton_clicked() {
    showCover();
    DesktopWm::setScreenOff(true);
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
    connect(opacity, &tPropertyAnimation::finished, [ = ] {
        tVariantAnimation* anim = new tVariantAnimation();
        anim->setStartValue((float) 1);
        anim->setEndValue((float) 0);
        anim->setDuration(250);
        anim->setEasingCurve(QEasingCurve::InCubic);
        connect(anim, &tVariantAnimation::valueChanged, [ = ](QVariant value) {
            this->setWindowOpacity(value.toFloat());
        });
        connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
        connect(anim, &tVariantAnimation::finished, [ = ] {
            this->close();
        });

        anim->start();
    });
    opacity->start();
    passUp->start();
}

void LockScreen::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_MediaTogglePlayPause || event->key() == Qt::Key_MediaPlay ||
        event->key() == Qt::Key_MediaPause || event->key() == Qt::Key_MediaStop) {
        d->slide->mprisPlayPause();
    } else if (event->key() == Qt::Key_MediaNext) {
        d->slide->mprisNext();
    } else if (event->key() == Qt::Key_MediaPrevious) {
        d->slide->mprisBack();
    } else if (event->key() == Qt::Key_Return && event->modifiers() == Qt::MetaModifier) {
        QDBusMessage m = QDBusMessage::createMethodCall("org.thesuite.theshell", "/org/thesuite/theshell", "org.thesuite.theshell", "NextKeyboard");
        QDBusConnection::sessionBus().call(m, QDBus::NoBlock);
    }
}

void LockScreen::on_SuspendButton_clicked() {
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

const QDBusArgument& operator<<(QDBusArgument& argument, const LoginSession& session) {
    argument.beginStructure();
    argument << session.sessionId << session.userId << session.username << session.seat << session.path;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, LoginSession& session) {
    argument.beginStructure();
    argument >> session.sessionId >> session.userId >> session.username >> session.seat >> session.path;
    argument.endStructure();
    return argument;
}

void LockScreen::on_switchUserButton_clicked() {
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

void LockScreen::on_passwordButton_toggled(bool checked) {
    if (checked) {
        ui->loginStack->setCurrentIndex(0);
        ui->password->setFocus();
    }
}

void LockScreen::on_mousePasswordButton_toggled(bool checked) {
    if (checked) {
        ui->loginStack->setCurrentIndex(1);
    }
}

void LockScreen::on_loginStack_currentChanged(int arg1) {
    switch (arg1) {
        case 0:
            ui->passwordButton->setChecked(true);
            break;
        case 1:
            ui->mousePasswordButton->setChecked(true);
            break;
    }
}

void LockScreen::on_retryMousePasswordButton_clicked() {
    if (currentMousePassword.length() > 5) {
        mousePasswordWrongCount++;
    }
    currentMousePassword.clear();
    checkMousePassword();
}

void LockScreen::on_backToLogin_clicked() {
    ui->pagesStack->setCurrentIndex(0);
}

void LockScreen::on_newSessionButton_clicked() {
    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.DisplayManager", QString(qgetenv("XDG_SEAT_PATH")), "org.freedesktop.DisplayManager.Seat", "SwitchToGreeter");
    QDBusConnection::systemBus().send(message);
    ui->pagesStack->setCurrentIndex(0);
    showCover();
}

void LockScreen::on_availableUsersList_itemActivated(QListWidgetItem* item) {
    ui->pagesStack->setCurrentIndex(0);
    showCover();

    QDBusMessage message = QDBusMessage::createMethodCall("org.freedesktop.login1", item->data(Qt::UserRole).toString(), "org.freedesktop.login1.Session", "Activate");
    QDBusConnection::systemBus().send(message);
}
