#pragma once

class MessageItem;
class ConversationMessage;
class MessagesHandler;

class MessageListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QString avatarData READ avatarData NOTIFY avatarDataChanged FINAL)
    Q_PROPERTY(QString participants READ participants NOTIFY participantsChanged FINAL)
    Q_PROPERTY(bool isSending READ isSending NOTIFY isSendingChanged FINAL)
    Q_PROPERTY(QString draftText READ draftText WRITE setDraftText NOTIFY draftTextChanged FINAL)
    Q_PROPERTY(QVector<QString> draftAttachments READ draftAttachments WRITE setDraftAttachments NOTIFY draftAttachmentsChanged FINAL)
    Q_PROPERTY(bool hasSendFailure READ hasSendFailure WRITE setHasSendFailure NOTIFY hasSendFailureChanged)

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
    bool isSending() const { return m_isSending; }
    QString draftText() const { return m_draftTexts.value(m_conversationID); }
    QVector<QString> draftAttachments() const { return m_draftAttachments.value(m_conversationID); }
    void setDevice(MessagesHandler *messagesHandlerForNewDevice);
    void setDraftText(const QString &draftText);
    void setDraftAttachments(const QVector<QString> &draftAttachments);
    bool hasSendFailure() const { return m_hasSendFailure; }
    void setHasSendFailure(bool hasSendFailure);

public slots:
    void setConversationID(qint64 conversationID);
    void sendMessage(const QString &message, const QVector<QUrl> &attachments, bool isDownscaling);
    QUrl turnClipboardIntoAttachment() const;

signals:
    void avatarDataChanged();
    void participantsChanged();
    void isSendingChanged();
    void draftTextChanged();
    void draftAttachmentsChanged();
    void hasSendFailureChanged();

private:
    void onConversationMessageChanged(const ConversationMessage &updatedMessage);
    void addOrUpdate(const ConversationMessage &date);
    void updateTimes();

    void onMessageDelivered(qint64 conversationID);
    void onMessageDeliveryFailed(qint64 conversationID);
    void setIsSending(bool isSending);

    MessagesHandler *m_messagesHandler = nullptr;
    qint64 m_conversationID = 0;
    QVector<MessageItem*> m_list;
    QSet<qint64> m_requestedConversations;
    QTimer m_updateTimesTimer;
    QString m_avatarData;
    QString m_participants;
    bool m_isSending = false;
    QHash<qint64, QString> m_draftTexts;
    QHash<qint64, QVector<QString>> m_draftAttachments;
    bool m_hasSendFailure = false;
};
