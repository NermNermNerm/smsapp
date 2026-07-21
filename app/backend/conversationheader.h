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

    Q_PROPERTY(bool isLatestOutgoing READ isLatestOutgoing NOTIFY isLatestOutgoingChanged)
    Q_PROPERTY(bool isLatestDraft READ isLatestDraft NOTIFY isLatestDraftChanged)
    Q_PROPERTY(bool isUnread READ isUnread NOTIFY isUnreadChanged)

public:
    explicit ConversationHeader(const ConversationMessage &latestMessage, DraftMessages &drafts, QObject *parent = nullptr);

    static QString computeParticipants(const ConversationMessage &latestMessage);

    QDateTime date() const;
    QString participants() const;
    QString avatarData() const { return m_avatarData; }
    QString latestMessageBody() const { return m_body; }
    QString shortFriendlyDate() const { return m_shortFriendlyDate; }
    bool isLatestOutgoing() const { return m_isLatestOutgoing; }
    bool isLatestDraft() const { return m_isLatestDraft; }
    bool isUnread() const { return m_isUnread; }

    void updateTime();

    void setIsLatestOutgoing(bool isLatestOutgoing);
    void setIsLatestDraft(bool isLatestDraft);
    void setIsUnread(bool isUnread);

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
    void isLatestOutgoingChanged();
    void isLatestDraftChanged();
    void isUnreadChanged();

private:
    void onDraftStatusChanged(qint64 conversationID);
    void updateState();

    ConversationMessage m_latestMessage;
    QString m_avatarData;
    QString m_participants;
    QString m_shortFriendlyDate;
    DraftMessages &m_drafts;

    QString m_latestMessageBody;
    bool m_isLatestOutgoing = false;
    bool m_isLatestDraft = false;
    bool m_isUnread = false;
    QString m_body;
};
