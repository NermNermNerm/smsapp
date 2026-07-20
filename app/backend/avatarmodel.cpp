#include "avatarmodel.h"
#include "kdeconnect_interfaces/conversationmessage.h"

AvatarModel::AvatarModel(QObject *parent)
    : QObject{parent}
{}

QString AvatarModel::getAvatarData(const ConversationMessage &message)
{
    QString result;

    QSet<QString> alreadySeen;
    for (const auto &address : message.addresses()) {
        auto newAddress = getAvatarData(address);
        if (!alreadySeen.contains(newAddress)) {
            if (!result.isEmpty())
                result += "\n";
            result += newAddress;
            alreadySeen.insert(newAddress);
        }
    }
    return result;
}

QString AvatarModel::getAvatarData(const ConversationAddress &address)
{
    // ConversationAddress::address() may contain newlines; strip them.
    QString s = address.address();
    s.replace('\n', ' ');
    return s.trimmed();
}

void AvatarModel::setParticipants(const QString &participants)
{
    if (m_participants == participants)
        return;

    m_participants = participants;

    const QStringList parts = participants.split('\n', Qt::SkipEmptyParts);
    m_participant1 = parts.size() > 0 ? parts[0] : "";
    m_participant2 = parts.size() > 1 ? parts[1] : "";
    m_participant3 = parts.size() > 2 ? parts[2] : "";
    m_participant4 = parts.size() > 3 ? parts[3] : "";

    emit participantsChanged();
}
