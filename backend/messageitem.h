#ifndef MESSAGEITEM_H
#define MESSAGEITEM_H

#include <QObject>
#include <QDateTime>
#include "kdeconnect_interfaces/conversationmessage.h"

class ConversationMessage;

/**
 * @brief Represents the model for a single message within a conversation.
 */
class MessageItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QDateTime date READ date CONSTANT)
    Q_PROPERTY(QString sender READ sender CONSTANT)
    Q_PROPERTY(bool isIncoming READ isIncoming CONSTANT)
    Q_PROPERTY(QString initials READ initials CONSTANT)
    Q_PROPERTY(QColor avatarBackground READ avatarBackground CONSTANT)
    Q_PROPERTY(QColor avatarForeground READ avatarForeground CONSTANT)
    Q_PROPERTY(QString body READ body NOTIFY bodyChanged FINAL)

public:
    explicit MessageItem(const ConversationMessage &message, QObject *parent = nullptr)
        : m_rawData(message), QObject(parent)
    { }

    void update(const ConversationMessage& updated);

    QDateTime date() const { return QDateTime::fromMSecsSinceEpoch(m_rawData.date()); }
    QString body() const { return m_rawData.body(); }
    QString sender() const;
    bool isIncoming() const { return m_rawData.isIncoming(); }
    QString initials() const;
    QColor avatarBackground() const;
    QColor avatarForeground() const;

    const ConversationMessage &rawData() const { return m_rawData; }

signals:
    void bodyChanged();

private:
    ConversationMessage m_rawData;
    QString m_cachedRecipientList;
    mutable QString m_sender;
};

#endif // MESSAGEITEM_H
