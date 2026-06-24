#include "conversationlistmodel.h"
#include "conversationheader.h"
#include "kdeconnect_interfaces/conversationmessage.h"
#include "messageshandler.h"

ConversationListModel::ConversationListModel(QObject *parent)
    : QAbstractListModel{parent}
{}

void ConversationListModel::setDevice(MessagesHandler *messagesHandlerForNewDevice)
{
    m_messagesHandler = messagesHandlerForNewDevice;
    m_messagesHandler->connect(m_messagesHandler, &MessagesHandler::conversationMessageCountChanged
                               , this, &ConversationListModel::onConversationMessageCountChanged);
    // Note - the old device (if it exists) will have already been destroyed, so we don't
    // have to worry about detaching from it.

    // It's the caller's job to call MessagesHandler::startListening after this call.

    beginRemoveRows(QModelIndex(), 0, m_list.size());
    m_list.clear();
    m_index.clear();
    endRemoveRows();

    // We actually *can't* ask for a cached list of conversations here, because conversations can be deleted
    // on the phone and KDE doesn't/can't tell us about that through any other mechanism than requestAllConversations.
}

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

QHash<int, QByteArray> ConversationListModel::roleNames() const
{
    return {
        { ObjectRole, "object" }
    };
}

void ConversationListModel::onConversationMessageCountChanged(qint64 conversationID, int messageCount)
{
    m_messagesHandler->requestConversationItem(
        conversationID,
        1,
        [this](int index, const ConversationMessage &message) {
            auto *c = m_index.value(message.threadID());
            if (c) {
                c->update(message);
            }
            else {
                auto *newConversation = new ConversationHeader(message, this);
                beginInsertRows(QModelIndex(), m_list.size(), m_list.size());
                m_list.append(newConversation);
                endInsertRows();
                m_index.insert(message.threadID(), newConversation);
            }
        });
}
