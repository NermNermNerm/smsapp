#include "messagelistmodel.h"
#include "messageitem.h"
#include "kdeconnect_interfaces/conversationmessage.h"
#include "kdeconnect_interfaces/conversationmessage_ext.h"
#include "backend/messageshandler.h"
#include <QVector>
#include <QDebug>
#include "backend/avatarmodel.h"
#include "backend/conversationheader.h"

// Use QVector internally (Qt6 best practice)
MessageListModel::MessageListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&m_updateTimesTimer, &QTimer::timeout, this, &MessageListModel::updateTimes);
    m_updateTimesTimer.setInterval(30000);
    m_updateTimesTimer.start();
}

// ---------------------------------------------------------------
// Required overrides
// ---------------------------------------------------------------

int MessageListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_list.size();
}

QVariant MessageListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    int row = index.row();
    if (row < 0 || row >= m_list.size())
        return {};

    MessageItem *item = m_list[row];

    switch (role) {
    case ObjectRole:
        return QVariant::fromValue(item);
    default:
        return {};
    }
}

QHash<int, QByteArray> MessageListModel::roleNames() const
{
    return {
        { ObjectRole, "object" }
    };
}

void MessageListModel::setDevice(MessagesHandler *messagesHandlerForNewDevice)
{
    // The old m_messagesHandler, if it ever existed, should have been deleted before this,
    // meaning there's no need to disconnect from the old one.
    m_messagesHandler = messagesHandlerForNewDevice;
    QObject::connect(m_messagesHandler, &MessagesHandler::conversationMessageChanged,
                     this, &MessageListModel::onConversationMessageChanged);


    setConversationID(-1);
}

void MessageListModel::setConversationID(qint64 conversationID)
{
    if (conversationID == m_conversationID) {
        return;
    }

    // Clear old data
    beginResetModel();
    qDeleteAll(m_list); // if m_list holds pointers/owned objects; adjust as needed
    m_list.clear();
    endResetModel();

    if (conversationID < 0) {
        return;
    }

    // Populate the list from the cache
    //   First calculate all the latest messages from each thread
    const auto allConversationMessages = m_messagesHandler->getConversationMessages(conversationID);
    for (const ConversationMessage & message: allConversationMessages) {
        addOrUpdate(message);
    }

    if (!m_requestedConversations.contains(conversationID))
    {
        m_requestedConversations.insert(conversationID);
        m_messagesHandler->requestConversationItems(conversationID);
    }

    QString avatarData = "";
    QString participants = "";
    if (allConversationMessages.length() > 0) {
        auto sampleMessage = allConversationMessages.first();
        avatarData = AvatarModel::getAvatarData(sampleMessage);
        participants = ConversationHeader::computeParticipants(sampleMessage);
    }

    if (avatarData != m_avatarData) {
        m_avatarData = avatarData;
        emit avatarDataChanged();
    }
    if (participants != m_participants) {
        m_participants = participants;
        emit participantsChanged();
    }
}

void MessageListModel::onConversationMessageChanged(const ConversationMessage &updatedMessage)
{
    addOrUpdate(updatedMessage);
}

void MessageListModel::addOrUpdate(const ConversationMessage &updatedMessage)
{
    int i = 0; // i is a candidate insert position

    // Scan until either:
    //  - we find the same message (update + return)
    //  - we find a message older than updatedMessage (insert before it)
    for (;  i < m_list.size() && !isNewerMessage(updatedMessage, m_list[i]->rawData()); ++i) {
        MessageItem *item = m_list[i];
        if (isSameMessageID(updatedMessage, item->rawData())) {
            item->update(updatedMessage);
            return;
        }
    }

    // Now i is the insert position
    beginInsertRows(QModelIndex(), i, i);
    auto *newMessage = new MessageItem(updatedMessage, this);
    m_list.insert(i, newMessage);
    endInsertRows();

    newMessage->updateShowTime(i < m_list.size()-1 ? m_list[i+1]->date() : QDateTime());
    if (i > 0) {
        m_list[i-1]->updateShowTime(newMessage->date());
    }
}

void MessageListModel::updateTimes()
{
    const int numItems = m_list.size();
    for (int i = 0; i < numItems; ++i) {
        const QDateTime prior =
            (i + 1 < numItems ? m_list[i + 1]->date() : QDateTime{});

        m_list[i]->updateShowTime(prior);
    }
}
