#pragma once
#include "kdeconnect_interfaces/conversationmessage.h"
#include <QString>
#include <QList>

struct DeviceConfig {
    QString id;
    QString name;
    bool reachable = false;
    QVector<ConversationMessage> smsMessages;

    static std::vector<std::unique_ptr<DeviceConfig>> load();
    void save();
    static DeviceConfig create(const QString &name);
    void remove();
    static DeviceConfig *restore(const QString &id);

    enum Direction {
        Incoming,
        Outgoing
    };

    ConversationMessage makeMessage(const QStringList &addresses, const QString &body, Direction direction, const QVariantList &attachmentUrls);
    ConversationMessage makeMessage(qint64 conversationID, const QString &body, Direction direction, const QVariantList &attachmentUrls);

private:
    QList<Attachment> makeAttachments(const QVariantList &attachmentUrls);
};
