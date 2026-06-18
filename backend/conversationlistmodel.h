#pragma once
#include <QAbstractListModel>
#include <QObject>

class SmsConversation;
class ConversationMessage;

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

    void addOrUpdateConversation(const ConversationMessage &message);
    bool isConversationLoaded(qint64 threadId);

private:
    QVector<SmsConversation*> m_list;
    QHash<qint64, SmsConversation*> m_index;
};
