#include <QApplication>
#include <QWindow>
#include <QSound>
#include "mainwindow.h"
#include "notificationdbus.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationName("theSuite");
    a.setOrganizationDomain("");
    a.setApplicationName("tsscreenlock");

    XGrabKeyboard(QX11Info::display(), RootWindow(QX11Info::display(), 0), True, GrabModeAsync, GrabModeAsync, CurrentTime);
    XGrabPointer(QX11Info::display(), RootWindow(QX11Info::display(), 0), True, None, GrabModeAsync, GrabModeAsync, RootWindow(QX11Info::display(), 0), None, CurrentTime);

    NotificationDBus* notification = new NotificationDBus();

    QList<MainWindow*> windows;
    for (int i = 0; i < a.desktop()->screenCount(); i++) {
        MainWindow* w = new MainWindow();
        w->setWindowFlags(Qt::WindowStaysOnTopHint);
        QObject::connect(notification, SIGNAL(showNotification(QString,QString,uint,QStringList,QVariantMap)), w, SLOT(showNotification(QString,QString,uint,QStringList,QVariantMap)));
        w->show();
        w->setGeometry(a.desktop()->screenGeometry(i));
        //w->move(a.desktop()->screenGeometry(i).x(), a.desktop()->screenGeometry(i).y());
        w->showFullScreen();
        windows.append(w);
    }

    QSound* lockSound = new QSound(":/sounds/lock");
    lockSound->play();

    int ret = a.exec();

    for (MainWindow* w : windows) {
        w->close();
    }

    QSound* unlockSound = new QSound(":/sounds/unlock");
    unlockSound->play();
    while (!unlockSound->isFinished()) {
        a.processEvents();
    }

    return ret;
}
