#pragma once


#include "kdeconnect_proxy.h"

namespace dbus {

//
// Global configuration
//
inline QString serviceName = "org.kde.kdeconnect";

inline const QString daemonPath = "/modules/kdeconnect";
inline const QString deviceBasePath = "/modules/kdeconnect/devices/";
inline bool isUsingFakeDBus() { return serviceName != "org.kde.kdeconnect"; }

inline void init()
{
    // Build a raw DBus call to the fake daemon
    QDBusMessage msg = QDBusMessage::createMethodCall(
        "org.fake.kdeconnect",        // service
        daemonPath,                   // object path
        OrgKdeKdeconnectDaemonInterface::staticInterfaceName(),  // interface
        "reset"                       // method
        );

    QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block);

    if (reply.type() == QDBusMessage::ReplyMessage) {
        qDebug() << "Fake daemon responded to reset(); switching serviceName.";
        dbus::serviceName = "org.fake.kdeconnect";
    } else {
        qDebug() << "Fake daemon not available:"
                 << reply.errorName()
                 << reply.errorMessage();
    }
}

inline QDBusConnection bus()
{
    return QDBusConnection::sessionBus();
}

//
// Singleton daemon proxy
//
inline org::kde::kdeconnect::daemon& daemon()
{
    static org::kde::kdeconnect::daemon instance(
        serviceName,
        daemonPath,
        bus(),
        nullptr
        );
    return instance;
}

//
// Device proxy singletons (one per device ID)
//
inline org::kde::kdeconnect::device& device(const QString &id)
{
    static QHash<QString, org::kde::kdeconnect::device*> cache;

    if (!cache.contains(id)) {
        cache[id] = new org::kde::kdeconnect::device(
            serviceName,
            deviceBasePath + id,
            bus(),
            nullptr
            );
    }
    return *cache[id];
}

//
// Conversations proxy singletons
//
inline org::kde::kdeconnect::conversations& conversations(const QString &id)
{
    static QHash<QString, org::kde::kdeconnect::conversations*> cache;

    if (!cache.contains(id)) {
        cache[id] = new org::kde::kdeconnect::conversations(
            serviceName,
            deviceBasePath + id,
            bus(),
            nullptr
            );
    }
    return *cache[id];
}

//
// SMS proxy singletons
//
inline org::kde::kdeconnect::sms& sms(const QString &id)
{
    static QHash<QString, org::kde::kdeconnect::sms*> cache;

    if (!cache.contains(id)) {
        cache[id] = new org::kde::kdeconnect::sms(
            serviceName,
            deviceBasePath + id,
            bus(),
            nullptr
            );
    }
    return *cache[id];
}

//
// Telephony proxy singletons
//
inline org::kde::kdeconnect::telephony& telephony(const QString &id)
{
    static QHash<QString, org::kde::kdeconnect::telephony*> cache;

    if (!cache.contains(id)) {
        cache[id] = new org::kde::kdeconnect::telephony(
            serviceName,
            deviceBasePath + id,
            bus(),
            nullptr
            );
    }
    return *cache[id];
}

} // namespace dbus
