#ifndef LOCKSCREEN_H
#define LOCKSCREEN_H

#include <QDialog>
#include <QX11Info>
#include <QDesktopWidget>
#include <QFrame>
#include <QTimer>
#include <QDateTime>
#include <QPaintEvent>
#include <QPainter>
#include <QSettings>
#include <QResizeEvent>
#include <QFileInfo>
#include <QSvgRenderer>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QProcess>
#include <QDBusConnectionInterface>
#include <tpropertyanimation.h>
#include <QLayout>
#include <QPushButton>
#include <QLabel>
#include <QMenu>
#include <QLineEdit>
#include <QGraphicsOpacityEffect>
#include <QStackedWidget>
#include <QDir>
#include <QListWidgetItem>
#include <unistd.h>
#include <ttoast.h>
#include "timercomplete.h"
#include "switchuserlistdelegate.h"

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XInput.h>
#undef KeyPress
#undef KeyRelease
#undef Bool

namespace Ui {
    class LockScreen;
}

struct LockScreenPrivate;
class LockScreen : public QDialog {
        Q_OBJECT

    public:
        explicit LockScreen(QWidget* parent = nullptr);
        ~LockScreen();

    public slots:
        void hideCover();

        void showCover();

        void showFullScreen();

        void showNotification(QString summary, QString body, uint id, QStringList actions, QVariantMap hints);

        void closeNotification(uint id, uint reason);

        void animateClose();

    private slots:
        void on_goBackButton_clicked();

        void on_unlockButton_clicked();

        void on_password_returnPressed();

        void on_TurnOffScreenButton_clicked();

        void on_SuspendButton_clicked();

        void on_switchUserButton_clicked();

        void on_passwordButton_toggled(bool checked);

        void on_mousePasswordButton_toggled(bool checked);

        void on_loginStack_currentChanged(int arg1);

        void checkMousePassword();

        void on_retryMousePasswordButton_clicked();

        void on_backToLogin_clicked();

        void on_newSessionButton_clicked();

        void on_availableUsersList_itemActivated(QListWidgetItem* item);

    private:
        Ui::LockScreen* ui;
        LockScreenPrivate* d;

        int notificationHeight = 0;
        QMap<uint, QFrame*> notificationFrames;
        QString actionToEmit = "";
        uint idToEmit = -1;

        TimerComplete* TimerNotificationDialog;
        uint closeTimerId;

        bool eventFilter(QObject* obj, QEvent* e);
        void keyPressEvent(QKeyEvent* event);

        QGraphicsOpacityEffect* passwordFrameOpacity;

        QString mousePassword;
        QByteArray currentMousePassword;
        int mousePasswordWrongCount = 0;

        QSettings settings;
};

#endif // LOCKSCREEN_H
