#include "smsmessageitemlist.h"
#include "smsmessageitem.h"
#include "kdeconnect_interfaces/conversationmessage.h"

#include <QVector>
#include <QDebug>

// Use QVector internally (Qt6 best practice)
SmsMessageItemList::SmsMessageItemList(QObject *parent)
    : QAbstractListModel(parent)
{
}

// ---------------------------------------------------------------
// Required overrides
// ---------------------------------------------------------------

int SmsMessageItemList::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_list.size();
}

QVariant SmsMessageItemList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    int row = index.row();
    if (row < 0 || row >= m_list.size())
        return {};

    SmsMessageItem *item = m_list[row];

    switch (role) {
    case ObjectRole:
        return QVariant::fromValue(item);
    default:
        return {};
    }
}

QHash<int, QByteArray> SmsMessageItemList::roleNames() const
{
    return {
        { ObjectRole, "object" }
    };
}

// ---------------------------------------------------------------
// Ordering comparator for binary search
// ---------------------------------------------------------------

static bool messageLessThan(const SmsMessageItem *a, const SmsMessageItem *b)
{
    // Primary: timestamp
    if (a->date() != b->date())
        return a->date() < b->date();

    // Secondary: uid
    if (a->uID() != b->uID())
        return a->uID() < b->uID();

    // Tertiary: subId
    return a->subID() < b->subID();
}

// ---------------------------------------------------------------
// addMessageItem: sorted insert + duplicate suppression
// ---------------------------------------------------------------

void SmsMessageItemList::addMessageItem(const ConversationMessage &message)
{
    // Create a temporary item for comparison
    SmsMessageItem *temp = new SmsMessageItem(message);

    // Binary search for insertion point
    int low = 0;
    int high = m_list.size();

    while (low < high) {
        int mid = (low + high) / 2;
        if (messageLessThan(temp, m_list[mid]))
            high = mid;
        else
            low = mid + 1;
    }

    int insertPos = low;

    // Check if the item already exists at insertPos - 1
    if (insertPos > 0) {
        SmsMessageItem *prev = m_list[insertPos - 1];
        if (prev->uID() == temp->uID() && prev->subID() == temp->subID()) {
            delete temp;
            return; // duplicate
        }
    }

    // Check if the item already exists at insertPos
    if (insertPos < m_list.size()) {
        SmsMessageItem *next = m_list[insertPos];
        if (next->uID() == temp->uID() && next->subID() == temp->subID()) {
            delete temp;
            return; // duplicate
        }
    }

    // Insert into model
    beginInsertRows(QModelIndex(), insertPos, insertPos);
    m_list.insert(insertPos, temp);
    endInsertRows();
}
