#pragma once

class ConversationMessage;

class OtpScanner : public QObject
{
    Q_OBJECT
public:

    static OtpScanner &instance();

signals:
    void otpReceived(const QString &code, const QString &actualSender, const QString &body, const QString &parsedSender);

private:
    OtpScanner();

    struct RequiredPhrases {
        int nearnessRequirement; // -1 for no requirement at all, 0 means directly adjacent
        QStringList phrases;
    };

    struct RecognitionRule
    {
        int codeLength = 0;
        enum CodeType { Digits, Alnum } codeType = Digits;
        QVector<RequiredPhrases> requiredPhrases;
    };

    struct Sender {
        QString displayName;
        QStringList phrases;
    };

    void compileSenders(QSettings &rules);
    void onMessageChanged(const ConversationMessage &message);
    QString cookPhrase(const QString &raw, QStringList &warnings);
    QString cookMessageBody(const QString &raw) const;
    void compileRules(QSettings &settings);
    /** @brief Returns the display name for the sender if one of rules matches or "" if none matched. */
    QString findSender(const QString &cookedMessageBody) const;
    /** @brief Returns the OTP code for the message if the rule matches or "" if it does not match. */
    QString doesRuleMatch(const QString &cookedMessageBody, const RecognitionRule &rule) const;

    QVector<RecognitionRule> m_recognitionRules;
    QVector<Sender> m_senders;
};
