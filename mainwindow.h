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

private:
    Ui::MainWindow *ui;

    void resizeEvent(QResizeEvent* event);
    void keyPressEvent(QKeyEvent* event);

    int moveY;
    bool typePassword = false;
    QSize imageSize;

    QString mprisCurrentAppName = "";
    QStringList mprisDetectedApps;

};

#endif // MAINWINDOW_H
