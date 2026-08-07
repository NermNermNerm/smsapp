#pragma once

class ConversationHeader;
class ConversationMessage;
class MessagesHandler;
class DraftMessages;
class DeviceStatus;

/**
 * @brief This class backs up the list of conversations in the UX.  It maintains a list
 *   of ConversationHeaders, which
 */
class ConversationListModel : public QAbstractListModel
{
    Q_OBJECT
public slots:
    void setSelectedConversationID(qint64 conversationID);

public:
    static ConversationListModel &instance();

    enum Roles {
        ObjectRole = Qt::UserRole + 1
    };

    // Required overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    explicit ConversationListModel(QObject *parent = nullptr);
    void onConversationMessageChanged(const ConversationMessage &updatedMessage);
    void onConversationDeleted(qint64 conversationId);
    int findInsertPosition(const QDateTime &date) const;
    void on30SecondTimeCheckTick();
    void onMessageHandlerChanged();

    DeviceStatus &deviceStatus() const;

    QTimer m_30SecondTimeCheckTimer;
    QVector<ConversationHeader*> m_list;
    QHash<qint64, ConversationHeader*> m_index;
    qint64 m_selectedConversationID;
};
