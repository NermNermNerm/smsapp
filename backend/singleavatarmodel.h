#include "pch.h"
#pragma once

class SingleAvatarModel : public QObject
{
    Q_OBJECT

    // phoneNumber shouldn't be displayed by the UI, it's there to be bound to and the reader is just there for completeness.
    Q_PROPERTY(QString phoneNumber READ phoneNumber WRITE setPhoneNumber NOTIFY contentChanged FINAL)
    Q_PROPERTY(QString initials READ initials NOTIFY contentChanged FINAL)
    Q_PROPERTY(QColor color READ color NOTIFY contentChanged FINAL)
    // resolvedName could be used as a tooltip; mainly just exposing it becaues we have it.
    Q_PROPERTY(QString resolvedName READ resolvedName NOTIFY contentChanged FINAL)

public:
    explicit SingleAvatarModel(QObject *parent = nullptr);


    QString phoneNumber() const { return m_phoneNumber; }
    void setPhoneNumber(const QString phoneNumber);

    QString initials() const { return m_initial; }
    QColor color() const { return m_color; }
    QString resolvedName() const { return m_resolvedName; }

signals:
    void contentChanged();

private:
    QString m_phoneNumber;
    QString m_resolvedName;
    QString m_initial;
    QColor m_color;
};
