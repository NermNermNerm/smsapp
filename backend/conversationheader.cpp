#include "conversationheader.h"
#include "nameresolver.h"
#include "kdeconnect_interfaces/conversationmessage_ext.h"
#include "backend/avatarmodel.h"

static QString shortFriendlyDate(const QDateTime &dt)
{
    QDateTime now = QDateTime::currentDateTime();
    qint64 secs = dt.secsTo(now);

    if (secs < 60 * 60) {
        // Under 1 hour → Xm
        int minutes = secs / 60;
        return QStringLiteral("%1m").arg(minutes);
    }

    if (secs < 60 * 60 * 4) {
        // Under 4 hours → Xhr
        int hours = secs / 3600;
        return QStringLiteral("%1hr").arg(hours);
    }

    if (dt.date() == now.date()) {
        // Today → 8:15 AM
        return dt.toString("h:mm AP");
    }

    if (dt.daysTo(now) < 7) {
        // Within last 7 days → Sun
        return dt.toString("ddd");
    }

    if (dt.date().year() == now.date().year()) {
        // This year → Jun 15
        return dt.toString("MMM d");
    }

    // Older → 8/25/21
    return dt.toString("M/d/yy");
}


ConversationHeader::ConversationHeader(const ConversationMessage &latestMessage, DraftMessages &drafts, QObject *parent)
    : QObject{parent}
    , m_latestMessage(latestMessage)
    , m_participants(computeParticipants(latestMessage))
    , m_shortFriendlyDate(::shortFriendlyDate(QDateTime::fromMSecsSinceEpoch(latestMessage.date())))
    , m_drafts(drafts)
{
    m_avatarData = AvatarModel::getAvatarData(latestMessage);

    connect(&m_drafts, &DraftMessages::draftTextChanged, this, &ConversationHeader::onDraftStatusChanged);
    connect(&m_drafts, &DraftMessages::draftAttachmentsChanged, this, &ConversationHeader::onDraftStatusChanged);
    updateState();
}

QString ConversationHeader::computeParticipants(const ConversationMessage &latestMessage)
{
    QStringList names;
    names.reserve(latestMessage.addresses().size());

    QSet<QString> alreadySeen;
    for (const auto &address : latestMessage.addresses()) {
        const QString &phoneNumber = address.address();
        if (!alreadySeen.contains(phoneNumber)) {
            alreadySeen.insert(phoneNumber);
            QString name = NameResolver::phoneNumberToName(phoneNumber);
            names.append(name.isEmpty() ? phoneNumber : name);
        }
    }

    if (names.size() == 1)
        return names.first();

    if (names.size() == 2)
        return names[0] + " and " + names[1];

    // Oxford comma style: A, B, and C
    QString last = names.takeLast();
    return names.join(", ") + ", and " + last;
}


QDateTime ConversationHeader::date() const
{
    return QDateTime::fromMSecsSinceEpoch(m_latestMessage.date());
}

QString ConversationHeader::participants() const
{
    return m_participants;
}

bool ConversationHeader::isUpdateNeeded(const ConversationMessage &message) {
    return message.threadID() == m_latestMessage.threadID() && !isNewerMessage(m_latestMessage, message);
}

void ConversationHeader::update(const ConversationMessage &message)
{
    m_latestMessage = message;
    m_shortFriendlyDate = ::shortFriendlyDate(QDateTime::fromMSecsSinceEpoch(message.date()));
    emit dateChanged();
    updateState();
}

void ConversationHeader::onDraftStatusChanged(qint64 conversationID)
{
    if (conversationID == this->conversationID()) {
        updateState();
    }
}

void ConversationHeader::setIsLatestOutgoing(bool isLatestOutgoing)
{
    if (isLatestOutgoing != m_isLatestOutgoing) {
        m_isLatestOutgoing = isLatestOutgoing;
        emit isLatestOutgoingChanged();
    }
}
void ConversationHeader::setIsLatestDraft(bool isLatestDraft)
{
    if (m_isLatestDraft != isLatestDraft) {
        m_isLatestDraft = isLatestDraft;
        emit isLatestDraftChanged();
    }
}

void ConversationHeader::updateState()
{
    QString body;

    if (m_drafts.containsDraft(conversationID())) {
        setIsLatestDraft(true);
        setIsLatestOutgoing(true);

        body = m_drafts.getDraftText(conversationID()).trimmed();
        if (body.isEmpty()) {

            // We don't /really/ know it's an image, but it's some kind of attachment and it seems
            //  like too much work to infer if it's an image or not.
            body = "<image>";
        }

        body = "You: " + body;
    }
    else {
        setIsLatestDraft(false);
        setIsLatestOutgoing(m_latestMessage.isOutgoing());

        body = m_latestMessage.body().trimmed();
        if (body.isEmpty() && !m_latestMessage.attachments().isEmpty()) {
            //                ^^ This clause should be guaranteed if body is empty; but safety first.
            body = m_latestMessage.attachments().constFirst().mimeType().startsWith("image/") ? "<image>" : "<file>";
        }
        if (m_latestMessage.isOutgoing()) {
            body = "You: " + body;
        }
    }

    if (body != m_body) {
        m_body = body;
        emit latestMessageBodyChanged();
    }
}