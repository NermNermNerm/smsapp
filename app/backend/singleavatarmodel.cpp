// clazy:excludeall=qcolor-from-literal
#include "singleavatarmodel.h"
#include "nameresolver.h"

SingleAvatarModel::SingleAvatarModel(QObject *parent)
    : QObject{parent}
{}

QColor SingleAvatarModel::colorForResolvedName(const QString &resolvedName)
{
    static const QVector<QColor> palette = {
        "#F44336", "#E91E63", "#9C27B0", "#3F51B5", "#2196F3",
        "#009688", "#4CAF50", "#FF9800", "#795548", "#607D8B"
    };

    int phoneNumberHash = qHash(resolvedName) % palette.size();
    return palette[phoneNumberHash];
}

void SingleAvatarModel::setPhoneNumber(const QString phoneNumber)
{
    if (phoneNumber == m_phoneNumber)
        return;

    m_phoneNumber = phoneNumber;
    // Note, if phoneNumber is empty, m_resolvedName will be empty as well.
    m_resolvedName = NameResolver::phoneNumberToName(phoneNumber);
    m_initial.clear();
    for (QChar c : std::as_const(m_resolvedName)) {
        if (c.isLetter()) {
            m_initial = QString(c).toUpper();
            break;
        }
    }

    m_color = colorForResolvedName(m_resolvedName);

    emit contentChanged();
}
