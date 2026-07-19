#pragma once

#include "kdeconnect_interfaces/conversationmessage.h"
#include "messagelistmodel.h"

class ConversationHeader : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QDateTime date READ date NOTIFY dateChanged)
    Q_PROPERTY(QString participants READ participants CONSTANT)
    Q_PROPERTY(QString avatarData READ avatarData CONSTANT)
    Q_PROPERTY(QString latestMessageBody READ latestMessageBody NOTIFY latestMessageBodyChanged)
    Q_PROPERTY(QString shortFriendlyDate READ shortFriendlyDate NOTIFY dateChanged)
    Q_PROPERTY(qint64 conversationID READ conversationID CONSTANT)

public:
    explicit ConversationHeader(const ConversationMessage &latestMessage, QObject *parent = nullptr);

    static QString computeParticipants(const ConversationMessage &latestMessage);

    QDateTime date() const;
    QString participants() const;
    QString avatarData() const { return m_avatarData; }
    QString latestMessageBody() const;
    QString shortFriendlyDate() const { return m_shortFriendlyDate; }

    /**
     * @brief Checks to see if this message is a newer message in the conversation this covers.
     * @return true if the given message is newer than what we have.
     */
    bool isUpdateNeeded(const ConversationMessage &message);

    void update(const ConversationMessage &message);
    qint64 conversationID() const { return m_latestMessage.threadID(); }

signals:
    void dateChanged();
    void latestMessageBodyChanged();

private:
    ConversationMessage m_latestMessage;
    QString m_avatarData;
    QString m_participants;
    QString m_shortFriendlyDate;
};
