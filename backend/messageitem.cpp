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

QString MessageItem::initials() const
{
    const QString s = sender();

    // Check if the string contains any Unicode letters
    bool hasLetter = false;
    for (QChar c : s) {
        if (c.isLetter()) {
            hasLetter = true;
            break;
        }
    }

    if (hasLetter) { // we assume it's a name if it has a letter
        QStringList parts = s.split(' ', Qt::SkipEmptyParts);
        if (parts.size() == 1)
            return parts[0].left(1).toUpper();
        return parts[0].left(1).toUpper() + parts[1].left(1).toUpper();
    }
    else { // else tell the UI to display a generic thing.
        return QString();
    }
}

QColor MessageItem::avatarBackground() const {
    static const QVector<QColor> palette = {
        "#F44336", "#E91E63", "#9C27B0", "#3F51B5", "#2196F3",
        "#009688", "#4CAF50", "#FF9800", "#795548", "#607D8B"
    };

    uint h = qHash(sender());
    return palette[h % palette.size()];
}

QColor MessageItem::avatarForeground() const {
    QColor bg = avatarBackground();
    int luminance = (0.299 * bg.red()) + (0.587 * bg.green()) + (0.114 * bg.blue());
    return luminance > 128 ? Qt::black : Qt::white;
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
