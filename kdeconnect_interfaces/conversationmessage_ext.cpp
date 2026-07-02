#include "conversationmessage_ext.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

// -----------------------------------------------------------------------------
// Attachment
// -----------------------------------------------------------------------------

QJsonObject toJson(const Attachment &att)
{
    QJsonObject o;
    o["partID"] = static_cast<qint64>(att.partID());
    o["mimeType"] = att.mimeType();
    o["encoded"] = att.base64EncodedFile();
    o["unique"] = att.uniqueIdentifier();
    return o;
}

Attachment attachmentFromJson(const QJsonObject &o)
{
    return Attachment(
        o["partID"].toVariant().toLongLong(),
        o["mimeType"].toString(),
        o["encoded"].toString(),
        o["unique"].toString()
        );
}

// -----------------------------------------------------------------------------
// ConversationAddress
// -----------------------------------------------------------------------------

QJsonObject toJson(const ConversationAddress &addr)
{
    QJsonObject o;
    o["address"] = addr.address();
    return o;
}

ConversationAddress addressFromJson(const QJsonObject &o)
{
    return ConversationAddress(o["address"].toString());
}

// -----------------------------------------------------------------------------
// ConversationMessage
// -----------------------------------------------------------------------------

QJsonObject toJson(const ConversationMessage &msg)
{
    QJsonObject o;

    o["eventField"] = msg.eventField();
    o["body"] = msg.body();
    o["date"] = static_cast<qint64>(msg.date());
    o["type"] = msg.type();
    o["read"] = msg.read();
    o["threadID"] = static_cast<qint64>(msg.threadID());
    o["uID"] = msg.uID();
    o["subID"] = static_cast<qint64>(msg.subID());

    // Addresses
    QJsonArray addrArray;
    for (const ConversationAddress &addr : msg.addresses()) {
        addrArray.append(toJson(addr));
    }
    o["addresses"] = addrArray;

    // Attachments
    QJsonArray attArray;
    for (const Attachment &att : msg.attachments()) {
        attArray.append(toJson(att));
    }
    o["attachments"] = attArray;

    return o;
}

ConversationMessage messageFromJson(const QJsonObject &o)
{
    // Addresses
    QList<ConversationAddress> addresses;
    for (const QJsonValue &v : o["addresses"].toArray()) {
        addresses.append(addressFromJson(v.toObject()));
    }

    // Attachments
    QList<Attachment> attachments;
    for (const QJsonValue &v : o["attachments"].toArray()) {
        attachments.append(attachmentFromJson(v.toObject()));
    }

    return ConversationMessage(
        o["eventField"].toInt(),
        o["body"].toString(),
        addresses,
        o["date"].toVariant().toLongLong(),
        o["type"].toInt(),
        o["read"].toInt(),
        o["threadID"].toVariant().toLongLong(),
        o["uID"].toInt(),
        o["subID"].toVariant().toLongLong(),
        attachments
        );
}


bool isSameMessageID(const ConversationMessage &oldMsg, const ConversationMessage &newMsgb)
{
    return oldMsg.threadID() == newMsgb.threadID()
        && oldMsg.date() == newMsgb.date()
        && oldMsg.uID() == newMsgb.uID()
        && oldMsg.subID() == newMsgb.subID();
}

bool isFullVersionOf(const ConversationMessage &oldMsg,
                       const ConversationMessage &newMsg)
{
    if (!isSameMessageID(oldMsg, newMsg))
        return false;

    // Full messages always have more content than metadata messages
    if (newMsg.body().length() > oldMsg.body().length())
        return true;

    if (newMsg.attachments().size() > oldMsg.attachments().size())
        return true;

    return false;
}

bool isNewerMessage(const ConversationMessage &a, const ConversationMessage &b)
{
    if (a.date() != b.date()) return a.date() > b.date();
    if (a.uID()  != b.uID())  return a.uID()  > b.uID();
    return a.subID() > b.subID();
}