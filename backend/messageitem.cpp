// clazy:excludeall=qcolor-from-literal
#include "messageitem.h"
#include "nameresolver.h"
#include <QSet>
#include <QColor>

QString MessageItem::sender() const
{
    if (m_sender.isEmpty()) {
        auto addresses = m_rawData.addresses();
        m_sender = NameResolver::phoneNumberToName(addresses.first().address());
    }
    return m_sender;
}

void MessageItem::update(const ConversationMessage& updated)
{
    auto old = m_rawData;
    m_rawData = updated;
    if (old.body() != m_rawData.body()) {
        emit bodyChanged();
    }
    // Nothing else should change
    Q_ASSERT(old.date() == updated.date()); // Sanity check
}

MessageItem::DisplayFormat MessageItem::computeDisplayFormat(QDateTime now) const
{
    if (m_displayFormat == DisplayFormat::FullDateTime) {
        // This won't be changing
        return m_displayFormat;
    }
    // Consider adding a similar short-circuit if m_displayFormat==MonthDayTime
    //  It seems not worth the bother as it still requires date calculations
    //  to do properly.

    const QDateTime msgDt = date();
    const QDate msgDate = msgDt.date();
    const QDate today = now.date();

    if (msgDt.secsTo(now) < 3600) {
        return DisplayFormat::RelativeToNow;
    }

    if (msgDate == today)
        return DisplayFormat::TodayTime;

    if (msgDate == today.addDays(-1))
        return DisplayFormat::YesterdayTime;

    // same week and same year -> weekday
    int msgWeek = msgDate.weekNumber();
    int todayWeek = today.weekNumber();
    if (msgWeek == todayWeek && msgDate.year() == today.year())
        return DisplayFormat::WeekdayTime;

    // same year -> MonthDayTime
    if (msgDate.year() == today.year())
        return DisplayFormat::MonthDayTime;

    // different year -> full
    return DisplayFormat::FullDateTime;
}

void MessageItem::updateShowTime(QDateTime priorMessage, QDateTime now)
{
    // store input
    m_priorMessageDate = priorMessage;

    const QDateTime msgDt = date();
    const qint64 ageSecs = msgDt.secsTo(now);

    // compute new format class using the same deterministic logic
    const DisplayFormat newFormat = computeDisplayFormat(now);

    // compute visibility (neighbor gap rules)
    bool newVisible = true;
    if (!priorMessage.isValid()) {
        newVisible = true;
    } else {
        const qint64 gapSecs = priorMessage.secsTo(msgDt);
        if (gapSecs < 0) {
            newVisible = true; // out-of-order -> show
        } else if (ageSecs < 3600) {
            newVisible = true; // recent messages show
        } else if (gapSecs < 300) {
            newVisible = false; // suppress if within 5 minutes
        } else {
            newVisible = true; // default show
        }
    }

    if (newFormat != m_displayFormat // We've changed date formats
        || newVisible != m_isDisplayDateVisible
        || formatDependsOnNow(newFormat)) {

        // commit: store format and visibility, clear cached string so displayDate() recomputes lazily
        m_displayFormat = newFormat;
        m_isDisplayDateVisible = newVisible;
        m_displayDate.clear();

        emit displayDateChanged();
    }
}

QString MessageItem::displayDate() const
{
    return displayDate(QDateTime::currentDateTime());
}

QString MessageItem::displayDate(QDateTime now) const
{
    // return cached if present
    if (!m_displayDate.isEmpty())
        return m_displayDate;

    // Now format according to stored format
    const QDateTime dt = date();
    QString formatted;
    if (m_displayFormat == DisplayFormat::Unknown) {
        m_displayFormat = computeDisplayFormat(now);
    }

    switch (m_displayFormat) {
    case DisplayFormat::TodayTime:
        formatted = dt.toString("h:mm ap");
        break;
    case DisplayFormat::YesterdayTime:
        formatted = QStringLiteral("Yesterday, ") + dt.toString("h:mm ap");
        break;
    case DisplayFormat::WeekdayTime:
        formatted = dt.toString("dddd, h:mm ap");
        break;
    case DisplayFormat::MonthDayTime:
        formatted = dt.toString("dddd, MMM d - h:mm ap");
        break;
    case DisplayFormat::FullDateTime:
        formatted = dt.toString("dddd, MMM d yyyy - h:mm ap");
        break;
    case DisplayFormat::RelativeToNow: {
        int minutes = dt.secsTo(now)/60;
        if (minutes <= 0){
            formatted = "just now";
        }
        else {
            formatted = QStringLiteral("%1m").arg(minutes);
        }
        break;
    }
    case DisplayFormat::Unknown:
        Q_UNREACHABLE();
    }

    m_displayDate = formatted;
    return m_displayDate;
}
