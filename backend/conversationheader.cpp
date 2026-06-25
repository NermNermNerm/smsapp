#include "conversationheader.h"
#include "nameresolver.h"
#include <QDateTime>

static QString computeParticipants(const ConversationMessage &latestMessage)
{
    QStringList names;
    names.reserve(latestMessage.addresses().size());

    QSet<QString> alreadySeen;
    for (const auto &address : latestMessage.addresses()) {
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

static QString shortFriendlyDate(const QDateTime &dt)
{
    QDateTime now = QDateTime::currentDateTime();
    qint64 secs = dt.secsTo(now);

    if (secs < 60 * 60) {
        // Under 1 hour → Xm
        int minutes = secs / 60;
        return QStringLiteral("%1m").arg(minutes);
    }

    if (secs < 60 * 60 * 4) {
        // Under 4 hours → Xhr
        int hours = secs / 3600;
        return QStringLiteral("%1hr").arg(hours);
    }

    if (dt.date() == now.date()) {
        // Today → 8:15 AM
        return dt.toString("h:mm AP");
    }

    if (dt.daysTo(now) < 7) {
        // Within last 7 days → Sun
        return dt.toString("ddd");
    }

    if (dt.date().year() == now.date().year()) {
        // This year → Jun 15
        return dt.toString("MMM d");
    }

    // Older → 8/25/21
    return dt.toString("M/d/yy");
}


ConversationHeader::ConversationHeader(const ConversationMessage &latestMessage, QObject *parent)
    : QObject{parent}
    , m_latestMessage(latestMessage)
    , m_participants(computeParticipants(latestMessage))
    , m_shortFriendlyDate(::shortFriendlyDate(QDateTime::fromMSecsSinceEpoch(latestMessage.date())))
{
}

QDateTime ConversationHeader::date() const
{
    return QDateTime::fromMSecsSinceEpoch(m_latestMessage.date());
}

QString ConversationHeader::participants() const
{
    return m_participants;
}

QString ConversationHeader::latestMessageBody() const
{
    return m_latestMessage.body();
}

qint64 ConversationHeader::threadID() const
{
    return m_latestMessage.threadID();
}

void ConversationHeader::update(const ConversationMessage &message)
{
    m_latestMessage = message;
    m_shortFriendlyDate = ::shortFriendlyDate(QDateTime::fromMSecsSinceEpoch(message.date()));
    emit latestMessageBodyChanged();
    emit dateChanged();
}