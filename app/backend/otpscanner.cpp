#include "otpscanner.h"
#include "backend/devicestatus.h"
#include "kdeconnect_interfaces/conversationmessage.h"
#include "backend/nameresolver.h"
#include "TrayIconController.h"

static void layDownDefaultSettings()
{
    // should be ~/.config/NermNermNerm
    QString dir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                   + "/" + QCoreApplication::organizationName();
    QDir().mkpath(dir);

    QString path = dir + "/two-factor-rules.ini";
    QFile f(path);
    if (f.exists())
        return;

    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Q_ASSERT(false);
        qWarning() << "Could not write default OTP settings file: " << path;
        return;
    }

    QFile res(":/resources/two-factor-rules.ini");
    bool openedResource = res.open(QIODevice::ReadOnly | QIODevice::Text);
    Q_ASSERT(openedResource);
    f.write(res.readAll());
    f.close();
}

OtpScanner::OtpScanner()
// ?    : m_settings(QSettings::IniFormat, QSettings::UserScope, "NermNermNerm", "twofactorrules")
{
    layDownDefaultSettings();
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QCoreApplication::organizationName(), "two-factor-rules");
    compileRules(settings);
    compileSenders(settings);

    connect( &DeviceStatus::instance(), &DeviceStatus::handlerChanged, this, [this]() {
        auto *handler = DeviceStatus::instance().handler();
        connect( handler, &MessagesHandler::conversationMessageChanged, this, &OtpScanner::onMessageChanged);
    });
}

QString OtpScanner::cookPhrase(const QString &raw, QStringList &warnings)
{
    QString s = raw;

    // Track punctuation
    bool hadPunct = false;

    // Convert punctuation → space
    for (QChar &c : s) {
        if (c.isPunct()) {
            hadPunct = true;
            c = ' ';
        }
    }

    if (hadPunct)
        warnings << "Phrase contains punctuation: \"" + raw + "\"";

    // Collapse whitespace
    s = s.simplified();

    // Empty after cleaning?
    if (s.isEmpty()) {
        warnings << "Phrase became empty after cleaning: \"" + raw + "\"";
        return QString();
    }

    // Add leading + trailing space for direct substring compare
    return " " + s.toCaseFolded() + " ";
}

QString OtpScanner::cookMessageBody(const QString &raw) const
{
    QString s = raw;
    for (QChar &c : s) {
        if (c.isPunct()) {
            c = ' ';
        }
    }

    // Add leading + trailing space for direct substring compare
    return " " + s.simplified().toCaseFolded() + " ";
}

void OtpScanner::compileRules(QSettings &rules)
{
    m_recognitionRules.clear();

    const QStringList groups = rules.childGroups();

    for (const QString &groupName : groups) {

        if (groupName == "sender-aliases")
            continue;

        rules.beginGroup(groupName);

        RecognitionRule rule;
        QStringList errors;

        // ----- codeLength -----
        if (!rules.contains("codeLength")) {
            errors << groupName + ": missing codeLength";
        } else {
            bool ok = false;
            int len = rules.value("codeLength").toInt(&ok);
            if (!ok || len <= 0)
                errors << groupName + ": invalid codeLength";
            else
                rule.codeLength = len;
        }

        // ----- codeType -----
        QString ct = rules.value("codeType").toString().trimmed().toLower();
        if (ct == "digits") {
            rule.codeType = RecognitionRule::Digits;
        } else if (ct == "alnum") {
            rule.codeType = RecognitionRule::Alnum;
        } else {
            errors << groupName + ": invalid codeType";
        }

        // ----- anywhere / near / nearN -----
        for (const QString &key : rules.childKeys()) {

            if (key == "codeLength" || key == "codeType")
                continue;

            QString val = rules.value(key).toString().trimmed();

            RequiredPhrases rp;

            // anywhere=
            if (key == "anywhere") {
                rp.nearnessRequirement = -1;
            }

            // near=
            else if (key == "near") {
                rp.nearnessRequirement = 2;   // near = within 3 words → 2 spaces
            }

            // nearN=
            else if (key.startsWith("near")) {
                bool ok = false;
                int N = QStringView{key}.mid(4).toInt(&ok);
                if (!ok || N <= 0) {
                    errors << groupName + ": bad nearN key: " + key;
                    continue;
                }
                rp.nearnessRequirement = N - 1;   // N words → N-1 spaces
            }

            // unknown key
            else {
                errors << groupName + ": unknown key: " + key;
                continue;
            }

            // ----- cook phrases -----
            QStringList rawGroup = val.split('|', Qt::SkipEmptyParts);
            QStringList cookedGroup;

            for (const QString &phrase : std::as_const(rawGroup)) {
                QString cooked = cookPhrase(phrase, errors);
                if (!cooked.isEmpty())
                    cookedGroup.append(cooked);
            }

            if (cookedGroup.isEmpty()) {
                errors << groupName + ": " + key + " has no usable phrases";
                continue;
            }

            rp.phrases = cookedGroup;
            rule.requiredPhrases.append(rp);
        }

        rules.endGroup();

        for (const QString &err : std::as_const(errors))
            qWarning() << "OTP rule error:" << err;

        if (errors.isEmpty())
            m_recognitionRules.append(rule);
    }
}


void OtpScanner::compileSenders(QSettings &rules)
{
    m_senders.clear();

    rules.beginGroup("sender-aliases");

    const QStringList keys = rules.childKeys();
    QStringList errors;

    for (const QString &key : keys) {
        QString displayName = key;
        displayName.replace('_', ' ');

        QStringList cookedList;
        cookedList << cookPhrase(displayName, errors);

        QStringList rawList = rules.value(key).toString().split('|', Qt::SkipEmptyParts);
        for (const QString &phrase : std::as_const(rawList)) {
            QString cooked = cookPhrase(phrase, errors);
            if (!cooked.isEmpty())
                cookedList.append(cooked);
        }

        m_senders << Sender { displayName, cookedList };
    }

    rules.endGroup();

    for (const QString &err : std::as_const(errors))
        qWarning() << "OTP sender alias error:" << err;
}

QString OtpScanner::findSender(const QString &cookedMessageBody) const
{
    for (const Sender &s : m_senders) {
        for (const QString &phrase : s.phrases) {
            if (cookedMessageBody.contains(phrase, Qt::CaseInsensitive)) {
                return s.displayName;
            }
        }
    }

    return QString();
}

QString OtpScanner::doesRuleMatch(const QString &cookedMessageBody,
                                  const RecognitionRule &rule) const
{
    // Find the OTP code -- note that we take the first one and don't keep looking if the first one
    // doesn't work out for us.  Maybe that's bad?  Prolly not.
    const int len = rule.codeLength;
    const bool digitsOnly = (rule.codeType == RecognitionRule::Digits);

    int codePos = -1;
    QString code;

    for (int i = 1; i + len < cookedMessageBody.size(); ++i) {
        if (cookedMessageBody[i - 1] != ' ' || cookedMessageBody[i + len] != ' ')
            continue;

        bool ok = true;
        QString candidate = cookedMessageBody.mid(i, len);
        bool hasDigit = false;
        for (QChar c : std::as_const(candidate)) {
            bool isDigit = c.isDigit();
            hasDigit |= isDigit;
            bool isValid = digitsOnly ? isDigit : c.isLetterOrNumber();
            if (!isValid) {
                ok = false;
                break;
            }
        }

        if (ok && hasDigit) {
            codePos = i;
            code = candidate;
            break;
        }
    }

    if (codePos < 0)
        return QString();   // no code found

    // Check required phrases
    // Check required phrases
    for (const RequiredPhrases &rp : rule.requiredPhrases) {

        bool matched = false;

        for (const QString &phrase : rp.phrases) {

            int from = 0;
            while (true) {
                int phrasePos = cookedMessageBody.indexOf(phrase, from);
                if (phrasePos < 0)
                    break;

                int phraseStart = phrasePos;
                int phraseEnd   = phrasePos + phrase.size();   // one past last char

                if (rp.nearnessRequirement < 0) {
                    matched = true;
                    break;
                }

                int rangeStart;
                int rangeEnd;

                if (phraseEnd < codePos) {
                    // phrase entirely left of code
                    rangeStart = phraseEnd;
                    rangeEnd   = codePos;
                } else if (codePos + rule.codeLength < phraseStart) {
                    // phrase entirely right of code
                    rangeStart = codePos + rule.codeLength + 1; // skip trailing space
                    rangeEnd   = phraseStart;
                } else {
                    // overlapping or touching
                    matched = true;
                    break;
                }

                int spaces = 0;
                for (int i = rangeStart; i < rangeEnd; ++i) {
                    if (cookedMessageBody[i] == ' ')
                        ++spaces;
                }

                if (spaces <= rp.nearnessRequirement) {
                    matched = true;
                    break;
                }

                from = phrasePos + 1;   // continue searching for next occurrence
            }

            if (matched)
                break;
        }

        if (!matched)
            return QString();   // required phrase group failed
    }

    // All required phrases matched
    return code;
}

OtpScanner &OtpScanner::instance()
{
    static auto *inst = new OtpScanner();
    return *inst;
}

static void copyToClipboard(const QString text)
{
    QGuiApplication::clipboard()->setText(text);
}

static void playOtpReceivedSound()
{

}

void OtpScanner::onMessageChanged(const ConversationMessage &message)
{
    if (message.date() < QDateTime::currentMSecsSinceEpoch() - 120'000)
        return;

    const QString cookedBody = cookMessageBody(message.body());
    for (const RecognitionRule &rule : std::as_const(m_recognitionRules)) {
        QString code = doesRuleMatch(cookedBody, rule);
        if (!code.isEmpty()) {
            QString parsedSender = findSender(cookedBody);
            auto addresses = message.addresses();
            QString actualSender = NameResolver::phoneNumberToName(addresses[0].address());

            // Mayyybeee there should be a clipboard controller?
            copyToClipboard(code);

            QGuiApplication::clipboard()->setText(code);
            emit otpReceived(code, actualSender, message.body(), parsedSender);
            // Toast.qml and SoundController watch for that.
            return;
        }
    }
}