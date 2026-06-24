#pragma once
#include <QAbstractListModel>
#include <QObject>

class ConversationHeader;
class ConversationMessage;
class MessagesHandler;

class ConversationListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit ConversationListModel(QObject *parent = nullptr);

    enum Roles {
        ObjectRole = Qt::UserRole + 1
    };

    // Required overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setDevice(MessagesHandler *messageHandlerForNewDevice);

private:
    void onConversationMessageCountChanged(qint64 conversationID, int messageCount);

    QVector<ConversationHeader*> m_list;
    QHash<qint64, ConversationHeader*> m_index;
    MessagesHandler *m_messagesHandler = nullptr;
};
