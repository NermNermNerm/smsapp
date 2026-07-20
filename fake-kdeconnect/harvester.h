#pragma once
#include "kdeconnect_interfaces/conversationmessage.h"
#include <QString>
#include <QVector>

namespace harvester {
    QVector<QString> readAllDeviceIds();
    QVector<qint64> readAllThreadIds(const QString &deviceId);
    QVector<ConversationMessage> readAllMessages(const QString &deviceId);
    QVector<ConversationMessage> readAllMessages(const QString &deviceId, qint64 threadId, const QString &fakeDeviceId);
}
