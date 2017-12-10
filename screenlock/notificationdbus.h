#ifndef NOTIFICATIONDBUS_H
#define NOTIFICATIONDBUS_H

#include <QObject>
#include <QMap>
#include <QVariant>

class NotificationDBus : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.thesuite.tsscreenlock.Notifications")

public Q_SLOTS:
    Q_SCRIPTABLE void newNotification(QString summary, QString body, uint id, QStringList actions, QVariantMap hints);

signals:
    void showNotification(QString summary, QString body, uint id, QStringList actions, QVariantMap hints);

public:
    NotificationDBus(QObject* parent = 0);
};

#endif // NOTIFICATIONDBUS_H
