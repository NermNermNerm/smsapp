#pragma once

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
    Q_PROPERTY(bool isDisplayDateVisible READ isDisplayDateVisible NOTIFY displayDateChanged FINAL)
    Q_PROPERTY(QString displayDate READ displayDate NOTIFY displayDateChanged FINAL)
    Q_PROPERTY(QString sender READ sender CONSTANT)
    Q_PROPERTY(bool isIncoming READ isIncoming CONSTANT)
    Q_PROPERTY(QString body READ body NOTIFY bodyChanged FINAL)
    Q_PROPERTY(QList<Attachment> attachments READ attachments CONSTANT)

public:
    explicit MessageItem(const ConversationMessage &message, QObject *parent = nullptr)
        : m_rawData(message), QObject(parent)
    { }

    void update(const ConversationMessage& updated);

    QDateTime date() const { return QDateTime::fromMSecsSinceEpoch(m_rawData.date()); }
    QString displayDate() const;
    QString displayDate(QDateTime now) const;
    bool isDisplayDateVisible() const { return m_isDisplayDateVisible; }
    QString body() const { return m_rawData.body(); }
    QString sender() const;
    bool isIncoming() const { return m_rawData.isIncoming(); }
    QList<Attachment> attachments() const { return m_rawData.attachments(); }

    const ConversationMessage &rawData() const { return m_rawData; }
    void updateShowTime(QDateTime priorMessage, QDateTime now = QDateTime::currentDateTime());

signals:
    void bodyChanged();
    void displayDateChanged();

private:
    enum class DisplayFormat {
        Unknown,
        RelativeToNow,
        TodayTime,
        YesterdayTime,
        WeekdayTime,
        MonthDayTime,
        FullDateTime
    };

    static bool formatDependsOnNow(DisplayFormat f) {
        return f == DisplayFormat::RelativeToNow
               || f == DisplayFormat::TodayTime
               || f == DisplayFormat::YesterdayTime;
    }
    DisplayFormat computeDisplayFormat(QDateTime priorMessage, QDateTime now) const;

    ConversationMessage m_rawData;
    QString m_cachedRecipientList;
    QDateTime m_priorMessageDate {};
    mutable DisplayFormat m_displayFormat { DisplayFormat::Unknown };
    mutable bool m_isDisplayDateVisible { false };
    mutable QString m_displayDate {};
    mutable QString m_sender;
};
