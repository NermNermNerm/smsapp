#include "smsconversation.h"
#include "smsmessageitem.h"

SmsConversation::SmsConversation(const ConversationMessage &firstMessage, QObject *parent)
    : QObject{parent}
{
    m_conversationList.addMessageItem(firstMessage);
}

QDateTime SmsConversation::date() const
{
    return m_conversationList.first().date();
}

QString SmsConversation::participants() const
{
    return m_conversationList.first().participants();
}

QString SmsConversation::latestMessageBody() const
{
    return m_conversationList.first().body();
}

qint32 SmsConversation::threadID() const
{
    return m_conversationList.first().threadID();
}


void SmsConversation::update(const ConversationMessage &message)
{
    QString oldLatestMessageBody = latestMessageBody();
    QDateTime oldDate = date();
    m_conversationList.addMessageItem(message);
    if (oldLatestMessageBody != latestMessageBody()) {
        emit latestMessageBodyChanged();
    }
    if (oldDate != date()) {
        emit dateChanged();
    }
}