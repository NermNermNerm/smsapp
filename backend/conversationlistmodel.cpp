#include "conversationlistmodel.h"
#include "smsconversation.h"
#include "kdeconnect_interfaces/conversationmessage.h"

ConversationListModel::ConversationListModel(QObject *parent)
    : QAbstractListModel{parent}
{}

int ConversationListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_list.size();
}

QVariant ConversationListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    if (role == ObjectRole)
        return QVariant::fromValue(m_list[index.row()]);

    return {};
}

bool ConversationListModel::isConversationLoaded(qint64 threadId)
{
    return m_index.contains(threadId);
}


QHash<int, QByteArray> ConversationListModel::roleNames() const
{
    return {
        { ObjectRole, "object" }
    };
}

void ConversationListModel::addOrUpdateConversation(const ConversationMessage &message)
{
    auto *c = m_index.value(message.threadID());
    if (c) {
        c->update(message);
        return;
    }

    auto *newConversation = new SmsConversation(message, this);
    beginInsertRows(QModelIndex(), m_list.size(), m_list.size());
    m_list.append(newConversation);
    endInsertRows();
    m_index.insert(message.threadID(), newConversation);
}
