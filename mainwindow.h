#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QX11Info>
#include <QProcess>
#include <QTimer>
#include <QDateTime>
#include <QDesktopWidget>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QSettings>
#include <QDBusReply>
#include <QDBusConnectionInterface>
#include <QFrame>
#include <QDebug>
#include <QMenu>
#include "newcall.h"

#include <X11/Xlib.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_lineEdit_returnPressed();

    void on_Cover_clicked();

    void on_Cover_MouseDown(QMouseEvent *);

    void on_Cover_MouseMove(QMouseEvent *);

    void on_Cover_MouseUp(QMouseEvent *);

    void hideCover();

    void showCover();

    void on_lineEdit_textEdited(const QString &arg1);

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

    void on_goBack_clicked();

    void mprisCheckTimer();

    void resizeSlot();

    void on_mprisPause_clicked();

    void on_mprisBack_clicked();

    void on_mprisNext_clicked();

    void showNotification(QString summary, QString body, uint id, QStringList actions, QVariantMap hints);

    void closeNotification(uint id, uint reason);

    void on_switchUser_clicked();

    void on_mprisSelection_triggered(QAction *arg1);

    void updateMpris();

    void on_stopTimerButton_clicked();

private:
    Ui::MainWindow *ui;

    QImage image;
    QImage darkImage;

    void resizeEvent(QResizeEvent* event);
    void keyPressEvent(QKeyEvent* event);

    int moveY;
    bool typePassword = false;
    QSize imageSize;

    QString mprisCurrentAppName = "";
    QStringList mprisDetectedApps;
    bool pauseMprisMenuUpdate = false;

    int notificationHeight = 0;
    QMap<uint, QFrame*> notificationFrames;
    QString actionToEmit = "";
    uint idToEmit = -1;

    uint closeTimerId;
};

#endif // MAINWINDOW_H
