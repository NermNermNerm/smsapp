#include "nameresolver.h"

#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QRegularExpression>

QHash<QString, QString> NameResolver::s_canonicalPhoneNumberToNameMap;

// ---------------------------------------------------------------
// Canonicalization: strip spaces, punctuation, plus signs, leading zeroes.
// One regex does the whole job.
// ---------------------------------------------------------------
static QString canonicalize(const QString &input)
{
    // Remove spaces, dashes, parentheses, plus signs
    static const QRegularExpression stripChars(R"([ \-\(\)\+])");

    // Remove leading zeroes
    static const QRegularExpression leadingZeroes(R"(^0+)");

    QString out = input;
    out.replace(stripChars, "");
    out.replace(leadingZeroes, "");

    if (out.isEmpty())
        return input;

    return out;
}

static QString canonicalizeEmail(const QString &raw)
{
    QString s = raw.trimmed();

    // Extract <...> if present
    int lt = s.indexOf('<');
    int gt = s.indexOf('>');
    if (lt != -1 && gt != -1 && gt > lt + 1) {
        s = s.mid(lt + 1, gt - lt - 1).trimmed();
    }

    // If no @, this is not a valid email → return as-is
    int at = s.indexOf('@');
    if (at == -1)
        return s;

    QString local = s.left(at);
    QString domain = s.mid(at + 1).toLower();  // domain is case-insensitive

    return local + "@" + domain;
}


// ---------------------------------------------------------------
// Load JSON from helper and populate the lookup table
// ---------------------------------------------------------------
void NameResolver::load()
{
    QProcess proc;
    proc.start("/home/steve/repos/kpeople_lookup/build/kpeople-lookup");   // adjust path if needed

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

        // Emails
        for (const QJsonValue &e : obj.value("emails").toArray()) {
            const QString email = e.toString();
            const QString canon = canonicalizeEmail(email);

            if (!canon.contains('@')) {
                qWarning() << "Suspicious email entry:" << email;
            }

            if (s_canonicalPhoneNumberToNameMap.contains(canon)) {
                qWarning() << "Duplicate canonical key:" << canon
                           << "existing:" << s_canonicalPhoneNumberToNameMap.value(canon)
                           << "new:" << name;
            }

            s_canonicalPhoneNumberToNameMap.insert(canon, name);
        }

        // Phone numbers (raw, unnormalized)
        for (const QJsonValue &p : obj.value("phones").toArray()) {
            const QString phone = p.toString();
            const QString canon = canonicalize(phone);

            if (s_canonicalPhoneNumberToNameMap.contains(canon)) {
                qWarning() << "Duplicate canonical key:" << canon
                           << "existing:" << s_canonicalPhoneNumberToNameMap.value(canon)
                           << "new:" << name;
            }

            s_canonicalPhoneNumberToNameMap.insert(canon, name);
        }
    }
}

// ---------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------
QString NameResolver::phoneNumberToName(const QString &phoneNumber)
{
    const QString canon = canonicalize(phoneNumber);

    if (auto it = s_canonicalPhoneNumberToNameMap.find(canon);
        it != s_canonicalPhoneNumberToNameMap.end()) {
        return *it;
    }

    return phoneNumber;
}
