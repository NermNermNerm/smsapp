#include "conversationheader.h"
#include "nameresolver.h"

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

ConversationHeader::ConversationHeader(const ConversationMessage &latestMessage, QObject *parent)
    : QObject{parent}
    , m_latestMessage(latestMessage)
    , m_participants(computeParticipants(latestMessage))
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
    emit latestMessageBodyChanged();
    emit dateChanged();
}