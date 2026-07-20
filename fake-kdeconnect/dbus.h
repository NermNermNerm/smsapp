#pragma once

#include <QString>
#include <QHash>
#include <QDBusConnection>

#include "kdeconnect_interfaces/kdeconnect_proxy.h"

namespace dbus {

//
// Global configuration
//
inline QString serviceName = "org.kde.kdeconnect";
// For testing:
// inline QString serviceName = "org.fake.kdeconnect";

inline const QString daemonPath = "/modules/kdeconnect";
inline const QString deviceBasePath = "/modules/kdeconnect/devices/";

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
            deviceBasePath + id + "/sms",
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
            deviceBasePath + id + "/telephony",
            bus(),
            nullptr
            );
    }
    return *cache[id];
}

} // namespace dbus
