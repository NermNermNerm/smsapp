#pragma once
#include <QAbstractListModel>
#include <QObject>
#include <QSet>

class MessageItem;
class ConversationMessage;
class MessagesHandler;

class MessageListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit MessageListModel(QObject *parent = nullptr);

    enum Roles {
        ObjectRole = Qt::UserRole + 1
    };

    // Required overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void setDevice(MessagesHandler *messagesHandlerForNewDevice);
    void setConversationID(qint64 conversationID);

private:
    void onConversationMessageChanged(const ConversationMessage &updatedMessage);
    void addOrUpdate(const ConversationMessage &date);

    MessagesHandler *m_messagesHandler = nullptr;
    qint64 m_conversationID;
    QVector<MessageItem*> m_list;
    QSet<qint64> m_requestedConversations;
};
