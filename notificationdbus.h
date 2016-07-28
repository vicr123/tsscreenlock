#ifndef NOTIFICATIONDBUS_H
#define NOTIFICATIONDBUS_H

#include <QObject>

class NotificationDBus : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.thesuite.tsscreenlock.Notifications")

public Q_SLOTS:
    Q_SCRIPTABLE void newNotification(QString summary, QString body, uint id, QStringList actions);

signals:
    void showNotification(QString summary, QString body, uint id, QStringList actions);

public:
    NotificationDBus(QObject* parent = 0);
};

#endif // NOTIFICATIONDBUS_H
