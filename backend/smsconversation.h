#pragma once

#include <QObject>
#include <QDateTime>
#include "kdeconnect_interfaces/conversationmessage.h"
#include "smsmessageitemlist.h"

class SmsConversation : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QDateTime date READ date NOTIFY dateChanged)
    Q_PROPERTY(QString participants READ participants CONSTANT)
    Q_PROPERTY(QString latestMessageBody READ latestMessageBody NOTIFY latestMessageBodyChanged)
    Q_PROPERTY(SmsMessageItemList* messages READ messages CONSTANT)

public:
    explicit SmsConversation( const ConversationMessage &firstMessage, QObject *parent = nullptr);

    QDateTime date() const;
    QString participants() const;
    QString latestMessageBody() const;
    SmsMessageItemList* messages() { return &m_conversationList; }

    void update(const ConversationMessage &message);
    int threadID() const;
signals:
    void dateChanged();
    void latestMessageBodyChanged();

private:
    SmsMessageItemList m_conversationList{this};
};
