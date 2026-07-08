#pragma once
#include <QAbstractListModel>
#include <QObject>
#include <QSet>
#include <QTimer>

class MessageItem;
class ConversationMessage;
class MessagesHandler;

class MessageListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString avatarData READ avatarData NOTIFY avatarDataChanged FINAL)
    Q_PROPERTY(QString participants READ participants NOTIFY participantsChanged FINAL)

public:
    explicit MessageListModel(QObject *parent = nullptr);

    enum Roles {
        ObjectRole = Qt::UserRole + 1
    };

    // Required overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString avatarData() const { return m_avatarData; }
    QString participants() const { return m_participants; }
    void setDevice(MessagesHandler *messagesHandlerForNewDevice);

public slots:
    void setConversationID(qint64 conversationID);

signals:
    void avatarDataChanged();
    void participantsChanged();

private:
    void onConversationMessageChanged(const ConversationMessage &updatedMessage);
    void addOrUpdate(const ConversationMessage &date);
    void updateTimes();

    MessagesHandler *m_messagesHandler = nullptr;
    qint64 m_conversationID;
    QVector<MessageItem*> m_list;
    QSet<qint64> m_requestedConversations;
    QTimer m_updateTimesTimer;
    QString m_avatarData;
    QString m_participants;
};
