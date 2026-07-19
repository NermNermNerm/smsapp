#pragma once

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
    Q_PROPERTY(bool isSenderVisible READ isSenderVisible NOTIFY isSenderVisibleChanged FINAL)
    Q_PROPERTY(QString sender READ sender CONSTANT)
    Q_PROPERTY(QColor senderColor READ senderColor CONSTANT)
    Q_PROPERTY(bool isIncoming READ isIncoming CONSTANT)
    Q_PROPERTY(QString body READ body NOTIFY bodyChanged FINAL)
    Q_PROPERTY(QString richTextBody READ richTextBody NOTIFY bodyChanged FINAL)
    Q_PROPERTY(QList<Attachment> attachments READ attachments CONSTANT)

public:
    explicit MessageItem(const ConversationMessage &message, QObject *parent = nullptr);

    void update(const ConversationMessage& updated);

    QDateTime date() const { return QDateTime::fromMSecsSinceEpoch(m_rawData.date()); }
    QString displayDate() const;
    QString displayDate(QDateTime now) const;
    bool isDisplayDateVisible() const { return m_isDisplayDateVisible; }
    QString body() const { return m_rawData.body(); }
    QString sender() const;
    QColor senderColor() const;
    bool isIncoming() const { return m_rawData.isIncoming(); }
    QList<Attachment> attachments() const { return m_rawData.attachments(); }
    QString richTextBody() const { return m_richTextBody; }
    bool isSenderVisible() const { return m_isSenderVisible; }

    const ConversationMessage &rawData() const { return m_rawData; }
    void updateShowTime(const ConversationMessage *priorMessage, QDateTime now = QDateTime::currentDateTime());

signals:
    void bodyChanged();
    void displayDateChanged();
    void isSenderVisibleChanged();

private:
    QString linkify(const QString &source) const;

    enum class DateDisplayFormat {
        Unknown,
        RelativeToNow,
        TodayTime,
        YesterdayTime,
        WeekdayTime,
        MonthDayTime,
        FullDateTime
    };

    static bool formatDependsOnNow(DateDisplayFormat f) {
        return f == DateDisplayFormat::RelativeToNow
               || f == DateDisplayFormat::TodayTime
               || f == DateDisplayFormat::YesterdayTime;
    }
    DateDisplayFormat computeDisplayFormat(QDateTime now) const;

    ConversationMessage m_rawData;
    QString m_cachedRecipientList;
    QDateTime m_priorMessageDate {};
    QString m_richTextBody;
    bool m_isDisplayDateVisible { false };
    bool m_isSenderVisible = false;

    mutable QString m_sender;
    mutable QColor m_senderColor;
    mutable DateDisplayFormat m_displayFormat { DateDisplayFormat::Unknown };
    mutable QString m_displayDate {};
};
