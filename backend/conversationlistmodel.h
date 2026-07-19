#pragma once

class ConversationHeader;
class ConversationMessage;
class MessagesHandler;
class DraftMessages;

/**
 * @brief This class backs up the list of conversations in the UX.  It maintains a list
 *   of ConversationHeaders, which
 */
class ConversationListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ConversationListModel(DraftMessages &drafts, QObject *parent = nullptr);

    enum Roles {
        ObjectRole = Qt::UserRole + 1
    };

    // Required overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setDevice(MessagesHandler *messageHandlerForNewDevice);

private:
    void onConversationMessageChanged(const ConversationMessage &updatedMessage);
    void onConversationDeleted(qint64 conversationId);
    int findInsertPosition(const QDateTime &date) const;

    QVector<ConversationHeader*> m_list;
    QHash<qint64, ConversationHeader*> m_index;
    MessagesHandler *m_messagesHandler = nullptr;
    DraftMessages &m_drafts;
};
