#include "smsmessageitem.h"
#include "nameresolver.h"
#include <QSet>

QString SmsMessageItem::participants() const
{
    QStringList names;
    names.reserve(m_rawData.addresses().size());

    QSet<QString> alreadySeen;
    for (const auto &address : m_rawData.addresses()) {
        const QString &phoneNumber = address.address();
        if (!alreadySeen.contains(phoneNumber)) {
            alreadySeen.insert(phoneNumber);
            QString name = NameResolver::phoneNumberToName(phoneNumber);
            names.append(name.isEmpty() ? phoneNumber : name);
        }
    }

    if (names.size() == 1)
        return names.first();

    if (names.size() == 2)
        return names[0] + " and " + names[1];

    // Oxford comma style: A, B, and C
    QString last = names.takeLast();
    return names.join(", ") + ", and " + last;
}