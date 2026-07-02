#include <QtAssert>
#include "conversationlistmodel.h"
#include "conversationheader.h"
#include "kdeconnect_interfaces/conversationmessage.h"
#include "kdeconnect_interfaces/conversationmessage_ext.h"
#include "messageshandler.h"

ConversationListModel::ConversationListModel(QObject *parent)
    : QAbstractListModel{parent}
{}

void ConversationListModel::setDevice(MessagesHandler *messagesHandlerForNewDevice)
{
    Q_ASSERT(messagesHandlerForNewDevice);

    // Hook up message handlers to the new guy
    // The old m_messagesHandler, if it ever existed, should have been deleted before this,
    // meaning there's no need to disconnect from the old one.
    m_messagesHandler = messagesHandlerForNewDevice;
    QObject::connect(m_messagesHandler, &MessagesHandler::conversationMessageChanged,
                     this, &ConversationListModel::onConversationMessageChanged);
    QObject::connect(m_messagesHandler, &MessagesHandler::conversationDeleted,
                     this, &ConversationListModel::onConversationDeleted);

    // Clear old data
    beginResetModel();
    qDeleteAll(m_list); // if m_list holds pointers/owned objects; adjust as needed
    m_list.clear();
    m_index.clear();
    endResetModel();

    // Populate the list from the cache
    //   First calculate all the latest messages from each thread
    QMap<qint64, const ConversationMessage *> latestMessageInThread;
    const auto allConversationMessages = m_messagesHandler->getAllConversationMessages();
    for (const ConversationMessage & message: allConversationMessages) {
        const ConversationMessage *existingMessage = latestMessageInThread[message.threadID()];
        if (!existingMessage || isNewerMessage(message, *existingMessage)) {
            latestMessageInThread[message.threadID()] = &message;
        }
    }

    if (!latestMessageInThread.isEmpty()) {
        //   Sort the conversations in reverse chronological order
        QVector<const ConversationMessage*> sortedConversations;
        sortedConversations.reserve(latestMessageInThread.size());
        for (auto it = latestMessageInThread.constBegin(); it != latestMessageInThread.constEnd(); ++it)
            sortedConversations.append(it.value());
        std::stable_sort(sortedConversations.begin(), sortedConversations.end(),
                         [](const ConversationMessage* a, const ConversationMessage* b) -> bool {
                             return isNewerMessage(*a, *b);
                         });

        // Build ConversationHeader model items out of that list.
        beginInsertRows(QModelIndex(), 0, sortedConversations.size()-1);
        for (const ConversationMessage *message: sortedConversations) {
            auto *header = new ConversationHeader(*message, this);
            m_list.emplaceBack(header);
            m_index.insert(message->threadID(), header);
        }
        endInsertRows();
    }
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

void ConversationListModel::onConversationMessageChanged(const ConversationMessage &updatedMessage)
{
    auto *associatedHeader = m_index[updatedMessage.threadID()];
    if (associatedHeader == nullptr) { // This is a new conversation
        auto *newConversation = new ConversationHeader(updatedMessage, this);
        int pos = findInsertPosition(QDateTime::fromMSecsSinceEpoch(updatedMessage.date()));
        beginInsertRows(QModelIndex(), pos, pos);
        m_list.insert(pos, newConversation);
        endInsertRows();
        m_index[updatedMessage.threadID()] = newConversation;
    }
    else if (associatedHeader->isUpdateNeeded(updatedMessage)) { // else if it's new data on an old one
        int oldRow = m_list.indexOf(associatedHeader);
        int newRow = findInsertPosition(QDateTime::fromMSecsSinceEpoch(updatedMessage.date()));
        if (newRow > oldRow)
            ++newRow;

        associatedHeader->update(updatedMessage);
        if (oldRow != newRow) {
            beginMoveRows(QModelIndex(), oldRow, oldRow,
                          QModelIndex(), newRow);
            m_list.move(oldRow, newRow);
            endMoveRows();
        }
    }
}

void ConversationListModel::onConversationDeleted(qint64 conversationId)
{
    auto *associatedHeader = m_index[conversationId];
    if (associatedHeader != nullptr)
    {
        int oldRow = m_list.indexOf(associatedHeader);
        beginRemoveRows(QModelIndex(), oldRow, oldRow);
        m_list.remove(oldRow);
        endRemoveRows();
        m_index.remove(conversationId);
        delete associatedHeader;
    }
}
