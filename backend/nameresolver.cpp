// clazy:excludeall=range-loop-detach
#include "nameresolver.h"

#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QRegularExpression>
#include <phonenumbers/phonenumberutil.h>
using namespace i18n::phonenumbers;

QHash<QString, QString> NameResolver::s_canonicalPhoneNumberToNameMap;

// ---------------------------------------------------------------
// Canonicalization: strip spaces, punctuation, plus signs, leading zeroes.
// One regex does the whole job.
// ---------------------------------------------------------------

static QString canonicalize(const QString &input)
{
    // Remove spaces, dashes, parentheses, plus signs
    static const QRegularExpression stripChars(R"([ \-\(\)\+])");

    // Remove leading zeroes (but not trunk prefixes; this is safe)
    static const QRegularExpression leadingZeroes(R"(^0+)");

    QString out = input.trimmed();
    out.replace(stripChars, "");
    out.replace(leadingZeroes, "");

    if (out.isEmpty())
        return input;

    // If original input started with "+", strip country code
    if (input.trimmed().startsWith("+")) {
        using namespace i18n::phonenumbers;

        PhoneNumberUtil *util = PhoneNumberUtil::GetInstance();
        PhoneNumber parsed;

        // Parse in international mode ("ZZ" = unknown region)
        auto err = util->Parse(input.toStdString(), "ZZ", &parsed);
        if (err == PhoneNumberUtil::NO_PARSING_ERROR) {
            int cc = parsed.country_code();
            QString ccStr = QString::number(cc);

            // Remove "+<cc>"
            if (out.startsWith(ccStr)) {
                out.remove(0, ccStr.length());
            }
        }

        return out;
    }

    // 2. Locale-based NANP rule (safe only for country code 1)
    QLocale systemLocale;
    int localeCountryCode = PhoneNumberUtil::GetInstance()->GetCountryCodeForRegion(
            systemLocale.name().split('_').last().toStdString()
        );

    // If locale is NANP (country code 1) and number begins with 1,
    // strip the leading 1. This is safe because NANP area codes
    // never begin with 1.
    if (localeCountryCode == 1 && out.startsWith("1")) {
        out.remove(0, 1);
        return out;
    }

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
    QString canon = canonicalize(phoneNumber);
    using namespace i18n::phonenumbers;

    if (auto it = s_canonicalPhoneNumberToNameMap.find(canon);
        it != s_canonicalPhoneNumberToNameMap.end()) {
        return *it;
    }

    return phoneNumber;
}
