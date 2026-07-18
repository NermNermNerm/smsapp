// clazy:excludeall=qcolor-from-literal
#include "messageitem.h"
#include "nameresolver.h"
#include <QSet>
#include <QColor>
#include <QRegularExpression>

MessageItem::MessageItem(const ConversationMessage &message, QObject *parent)
    : m_rawData(message), QObject(parent)
{
    m_richTextBody = linkify(message.body());
}

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
        m_richTextBody = linkify(m_rawData.body());
        emit bodyChanged();
    }
    // Nothing else should change
    Q_ASSERT(old.date() == updated.date()); // Sanity check
}

MessageItem::DateDisplayFormat MessageItem::computeDisplayFormat(QDateTime now) const
{
    if (m_displayFormat == DateDisplayFormat::FullDateTime) {
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
        return DateDisplayFormat::RelativeToNow;
    }

    if (msgDate == today)
        return DateDisplayFormat::TodayTime;

    if (msgDate == today.addDays(-1))
        return DateDisplayFormat::YesterdayTime;

    // same week and same year -> weekday
    int msgWeek = msgDate.weekNumber();
    int todayWeek = today.weekNumber();
    if (msgWeek == todayWeek && msgDate.year() == today.year())
        return DateDisplayFormat::WeekdayTime;

    // same year -> MonthDayTime
    if (msgDate.year() == today.year())
        return DateDisplayFormat::MonthDayTime;

    // different year -> full
    return DateDisplayFormat::FullDateTime;
}

void MessageItem::updateShowTime(QDateTime priorMessage, QDateTime now)
{
    // store input
    m_priorMessageDate = priorMessage;

    const QDateTime msgDt = date();
    const qint64 ageSecs = msgDt.secsTo(now);

    // compute new format class using the same deterministic logic
    const DateDisplayFormat newFormat = computeDisplayFormat(now);

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
    if (m_displayFormat == DateDisplayFormat::Unknown) {
        m_displayFormat = computeDisplayFormat(now);
    }

    switch (m_displayFormat) {
    case DateDisplayFormat::TodayTime:
        formatted = dt.toString("h:mm ap");
        break;
    case DateDisplayFormat::YesterdayTime:
        formatted = QStringLiteral("Yesterday, ") + dt.toString("h:mm ap");
        break;
    case DateDisplayFormat::WeekdayTime:
        formatted = dt.toString("dddd, h:mm ap");
        break;
    case DateDisplayFormat::MonthDayTime:
        formatted = dt.toString("dddd, MMM d - h:mm ap");
        break;
    case DateDisplayFormat::FullDateTime:
        formatted = dt.toString("dddd, MMM d yyyy - h:mm ap");
        break;
    case DateDisplayFormat::RelativeToNow: {
        int minutes = dt.secsTo(now)/60;
        if (minutes <= 0){
            formatted = "just now";
        }
        else {
            formatted = QStringLiteral("%1m").arg(minutes);
        }
        break;
    }
    case DateDisplayFormat::Unknown:
        Q_UNREACHABLE();
    }

    m_displayDate = formatted;
    return m_displayDate;
}

// SURELY there's a library function for this sort of thing.  It's straight-up
// AI-generated code, since I can't seem to find that library.
static QString escapeHref(const QString &url)
{
    QByteArray ba = url.toUtf8();
    QByteArray encoded;

    for (unsigned char c : ba) {
        // Safe characters per RFC 3986
        if ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~' ||
            c == '/' || c == ':' || c == '?' || c == '#' ||
            c == '[' || c == ']' || c == '@' ||
            c == '!' || c == '$' || c == '&' || c == '\'' ||
            c == '(' || c == ')' || c == '*' || c == '+' ||
            c == ',' || c == ';' || c == '=') {
            encoded.append(c);
        } else {
            encoded.append('%');
            encoded.append(QByteArray::number(c, 16).toUpper());
        }
    }

    return QString::fromUtf8(encoded);
}

QString MessageItem::linkify(const QString &input) const
{
    // Regex runs on ORIGINAL text
    static const QRegularExpression urlRegex(
        R"((https?://[^\s<]+))",
        QRegularExpression::CaseInsensitiveOption
        );

    QString result;
    result.reserve(input.size() * 1.2); // small optimization

    int lastPos = 0;

    QRegularExpressionMatchIterator it = urlRegex.globalMatch(input);
    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        int start = m.capturedStart(1);
        int end   = m.capturedEnd(1);
        QString rawUrl = m.captured(1);

        // --- Emit text BEFORE the URL (escaped + <br>) ---
        QString chunk = input.mid(lastPos, start - lastPos);
        chunk = chunk.toHtmlEscaped();
        chunk.replace("\n", "<br>");
        result += chunk;

        // --- Build <a href="...">...<a> ---
        // Escape ONLY for attribute context
        QString href = escapeHref(rawUrl);

        // Escape visible text for HTML content
        QString visible = rawUrl.toHtmlEscaped();

        result += QString("<a href=\"%1\">%2</a>").arg(href, visible);

        // Advance lastPos
        lastPos = end;
    }

    // --- Emit trailing text AFTER last match ---
    QString tail = input.mid(lastPos);
    tail = tail.toHtmlEscaped();
    tail.replace("\n", "<br>");
    result += tail;

    return result;
}