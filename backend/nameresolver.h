#pragma once
#include <QString>
#include <QHash>

class NameResolver
{
public:
    /// Called once at app startup to load the name lookup table.
    static void load();

    /// Attempt to find a name for the given phone number; returns the original phone number
    ///  if we can't find a matching contact.
    static QString phoneNumberToName(const QString &phoneNumber);

private:
    static QHash<QString, QString> s_canonicalPhoneNumberToNameMap;
};
