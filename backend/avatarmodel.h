#pragma once
#include <QObject>

class ConversationMessage;
class ConversationAddress;

class AvatarModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString participants READ participants WRITE setParticipants NOTIFY participantsChanged FINAL)
    Q_PROPERTY(QString participant1 READ participant1 NOTIFY participantsChanged FINAL)
    Q_PROPERTY(QString participant2 READ participant2 NOTIFY participantsChanged FINAL)
    Q_PROPERTY(QString participant3 READ participant3 NOTIFY participantsChanged FINAL)
    Q_PROPERTY(QString participant4 READ participant4 NOTIFY participantsChanged FINAL)
public:
    explicit AvatarModel(QObject *parent = nullptr);

    static QString getAvatarData(const ConversationMessage &message);
    static QString getAvatarData(const ConversationAddress &address);

    void setParticipants(const QString &participants);
    QString participants() const { return m_participants; }
    QString participant1() const { return m_participant1; }
    QString participant2() const { return m_participant2; }
    QString participant3() const { return m_participant3; }
    QString participant4() const { return m_participant4; }

signals:
    void participantsChanged();

private:
    QString m_participants;
    QString m_participant1;
    QString m_participant2;
    QString m_participant3;
    QString m_participant4;
};
