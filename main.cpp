#include <QApplication>
#include <QWindow>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    XGrabKeyboard(QX11Info::display(), RootWindow(QX11Info::display(), 0), True, GrabModeAsync, GrabModeAsync, CurrentTime);
    XGrabPointer(QX11Info::display(), RootWindow(QX11Info::display(), 0), True, None, GrabModeAsync, GrabModeAsync, RootWindow(QX11Info::display(), 0), None, CurrentTime);

    for (int i = 0; i < a.desktop()->screenCount(); i++) {
        MainWindow* w = new MainWindow();
        w->show();
        w->move(a.desktop()->screenGeometry(i).x(), a.desktop()->screenGeometry(i).y());
        w->showFullScreen();
    }



    return a.exec();
}
