#include "notificationdbus.h"
#include "tsscreenlock_adaptor.h"

NotificationDBus::NotificationDBus(QObject* parent) : QObject(parent)
{
    new NotificationsAdaptor(this);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject("/org/thesuite/tsscreenlock", this);
    dbus.registerService("org.thesuite.tsscreenlock");
}

void NotificationDBus::newNotification(QString summary, QString body, uint id) {
    emit showNotification(summary, body, id);
}
