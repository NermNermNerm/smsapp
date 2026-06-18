#ifndef SMSMESSAGEITEM_H
#define SMSMESSAGEITEM_H

#include <QObject>
#include <QDateTime>
#include "kdeconnect_interfaces/conversationmessage.h"

class ConversationMessage;

class SmsMessageItem : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QDateTime date READ date CONSTANT)
    Q_PROPERTY(QString body READ body CONSTANT)
    Q_PROPERTY(QString participants READ participants CONSTANT)

public:
    explicit SmsMessageItem(const ConversationMessage &message, QObject *parent = nullptr)
        : m_rawData(message), QObject(parent)
    { }

    QDateTime date() const { return QDateTime::fromMSecsSinceEpoch(m_rawData.date()); }
    QString body() const { return m_rawData.body(); }
    QString participants() const;

    qint32 threadID() const { return m_rawData.threadID(); }
    qint32 uID() const { return m_rawData.uID(); }
    qint32 subID() const { return m_rawData.subID(); }

signals:

private:
    ConversationMessage m_rawData;
    QString m_cachedRecipientList;
};

#endif // SMSMESSAGEITEM_H
