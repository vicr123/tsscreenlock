#include <QApplication>
#include <QWindow>
#include <QSound>
#include <QDebug>
#include <QTranslator>
#include <QLibraryInfo>
//#include "mainwindow.h"
#include "lockscreen.h"
#include "notificationdbus.h"

bool capturingKeyPress = false;
QList<LockScreen*> windows;
NotificationDBus* notification;

void openLockWindows() {
    for (LockScreen* w : windows) {
        w->close();
        //w->deleteLater();
    }
    windows.clear();

    for (int i = 0; i < QApplication::desktop()->screenCount(); i++) {
        LockScreen* w = new LockScreen();
        w->setWindowFlags(Qt::WindowStaysOnTopHint);
        QObject::connect(notification, SIGNAL(showNotification(QString,QString,uint,QStringList,QVariantMap)), w, SLOT(showNotification(QString,QString,uint,QStringList,QVariantMap)));
        w->show();
        w->setGeometry(QApplication::desktop()->screenGeometry(i));
        w->showFullScreen();
        windows.append(w);
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator *qtTranslator = new QTranslator;
    qtTranslator->load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    a.installTranslator(qtTranslator);

    QTranslator *tsTranslator = new QTranslator;
    tsTranslator->load(QLocale::system().name(), QString(SHAREDIR) + "translations");
    a.installTranslator(tsTranslator);

    qDebug() << a.arguments();
    if (!a.arguments().contains("--nograb") && !a.arguments().contains("-g")) {
        XGrabKeyboard(QX11Info::display(), RootWindow(QX11Info::display(), 0), True, GrabModeAsync, GrabModeAsync, CurrentTime);
        XGrabPointer(QX11Info::display(), RootWindow(QX11Info::display(), 0), True, None, GrabModeAsync, GrabModeAsync, RootWindow(QX11Info::display(), 0), None, CurrentTime);
    }

    a.setOrganizationName("theSuite");
    a.setOrganizationDomain("");
    a.setApplicationName("tsscreenlock");

    notification = new NotificationDBus();

    /*QList<MainWindow*> windows;
    for (int i = 0; i < a.desktop()->screenCount(); i++) {
        MainWindow* w = new MainWindow();
        w->setWindowFlags(Qt::WindowStaysOnTopHint);
        QObject::connect(notification, SIGNAL(showNotification(QString,QString,uint,QStringList,QVariantMap)), w, SLOT(showNotification(QString,QString,uint,QStringList,QVariantMap)));
        w->show();
        w->setGeometry(a.desktop()->screenGeometry(i));
        //w->move(a.desktop()->screenGeometry(i).x(), a.desktop()->screenGeometry(i).y());
        w->showFullScreen();
        windows.append(w);
    }*/

    openLockWindows();

    QObject::connect(QApplication::desktop(), &QDesktopWidget::screenCountChanged, &openLockWindows);
    QObject::connect(QApplication::desktop(), &QDesktopWidget::resized, &openLockWindows);

    QSound* lockSound = new QSound(":/sounds/lock");
    lockSound->play();

    int ret = a.exec();

    QSound* unlockSound = new QSound(":/sounds/unlock");
    unlockSound->play();

    for (LockScreen* w : windows) {
        w->animateClose();
    }

    while (!unlockSound->isFinished()) {
        a.processEvents();
    }

    return ret;
}

float getDPIScaling() {
    float currentDPI = QApplication::desktop()->logicalDpiX();
    return currentDPI / (float) 96;
}
