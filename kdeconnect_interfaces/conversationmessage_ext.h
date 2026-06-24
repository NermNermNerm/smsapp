#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include "conversationmessage.h"

QJsonObject toJson(const Attachment &att);
Attachment attachmentFromJson(const QJsonObject &obj);

QJsonObject toJson(const ConversationAddress &addr);
ConversationAddress addressFromJson(const QJsonObject &obj);

QJsonObject toJson(const ConversationMessage &msg);
ConversationMessage messageFromJson(const QJsonObject &obj);

bool isSameMessage(const ConversationMessage &a, const ConversationMessage &b);
bool isNewerMessage(const ConversationMessage &a, const ConversationMessage &b);
