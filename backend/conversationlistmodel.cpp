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

int ConversationListModel::findInsertPosition(const QDateTime &date) const
{
    for (int i = 0; i < m_list.size(); ++i) {
        if (date > m_list[i]->date())   // newest first
            return i;
    }
    return m_list.size(); // goes at end
}

void ConversationListModel::onConversationMessageCountChanged(qint64 conversationID, int messageCount)
{
    m_messagesHandler->requestConversationItem(
        conversationID,
        1,
        [this](int index, const ConversationMessage &message) {
            auto *c = m_index.value(message.threadID());
            if (c) {
                int oldRow = m_list.indexOf(c);
                if (oldRow == 0) {
                    // Guaranteed since this class only deals with the most recent message in the conversation.
                    Q_ASSERT(message.date() > m_list[0]->date().toMSecsSinceEpoch());
                    c->update(message);
                }
                else {
                    beginRemoveRows(QModelIndex(), oldRow, oldRow);
                    m_list.removeAt(oldRow);
                    endRemoveRows();

                    c->update(message);

                    int newRow = findInsertPosition(c->date());
                    beginInsertRows(QModelIndex(), newRow, newRow);
                    m_list.insert(newRow, c);
                    endInsertRows();
                }
            }
            else {
                auto *newConversation = new ConversationHeader(message, this);
                int pos = findInsertPosition(newConversation->date());
                beginInsertRows(QModelIndex(), pos, pos);
                m_list.insert(pos, newConversation);
                endInsertRows();

                m_index.insert(message.threadID(), newConversation);
            }
        });
}
