#pragma once

#include "conversationmessage.h"

QJsonObject toJson(const Attachment &att);
Attachment attachmentFromJson(const QJsonObject &obj);

QJsonObject toJson(const ConversationAddress &addr);
ConversationAddress addressFromJson(const QJsonObject &obj);

QJsonObject toJson(const ConversationMessage &msg);
ConversationMessage messageFromJson(const QJsonObject &obj);

/**
 * @brief returns true if the ID fields of the messages match.
 */
bool isSameMessageID(const ConversationMessage &a, const ConversationMessage &b);
/**
 * @brief returns true if the newMsg is a more complete message than oldMsg (e.g. oldMsg is a metadata-only version).
 */
bool isFullVersionOf(const ConversationMessage &oldMsg, const ConversationMessage &newMsg);
bool isNewerMessage(const ConversationMessage &oldMsg, const ConversationMessage &newMsg);
