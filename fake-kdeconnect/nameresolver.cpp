#include "nameresolver.h"
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QHash>
#include <QCoreApplication>

static QHash<QString, QString> s_nameToPhoneNumberMap;
static bool s_loaded;

static void load()
{
    if (s_loaded)
        return;

    QProcess proc;
    proc.start(QCoreApplication::applicationDirPath() + "/../../../kpeople_lookup/build/kpeople-lookup");

    if (!proc.waitForFinished(5000)) {
        qFatal("kpeople-lookup failed to run");
    }

    QByteArray raw = proc.readAllStandardOutput();
    QJsonDocument doc = QJsonDocument::fromJson(raw);

    if (!doc.isObject()) {
        qFatal("Invalid JSON from kpeople-lookup");
    }

    QJsonArray arr = doc.object().value("kpeopledata").toArray();

    for (const QJsonValue &v : arr) {
        QJsonObject obj = v.toObject();

        const QString name = obj.value("name").toString();

        // Phone numbers → forward + reverse
        for (const QJsonValue &p : obj.value("phones").toArray()) {
            const QString phone = p.toString();

            // reverse (first number wins)
            if (!s_nameToPhoneNumberMap.contains(name)) {
                s_nameToPhoneNumberMap.insert(name, phone);
            }
        }
    }

    s_loaded = true;
}

static bool looksLikePhoneNumber(const QString &s)
{
    bool hasDigit = false;

    for (QChar c : s) {
        if (c.isDigit())
            hasDigit = true;
        else if (!c.isSpace() && !QString("()+-").contains(c))
            return false;   // invalid character for phone number
    }

    return hasDigit;
}

QString lookupName(const QString &nameOrPhoneNumber)
{
    // 1. If it looks like a phone number, return it verbatim
    if (looksLikePhoneNumber(nameOrPhoneNumber))
        return nameOrPhoneNumber;

    // 2. Ensure lookup tables are loaded
    if (!s_loaded)
        load();

    // 3. Case-insensitive prefix match on names
    const QString needle = nameOrPhoneNumber.trimmed().toLower();

    for (auto it = s_nameToPhoneNumberMap.constBegin();
         it != s_nameToPhoneNumberMap.constEnd(); ++it)
    {
        if (it.key().toLower().startsWith(needle))
            return it.value();
    }

    // 4. No match
    return QString();
}
