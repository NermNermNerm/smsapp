#pragma once

class QSoundEffect;

class SoundController : public QObject
{
    Q_OBJECT
public:
    static SoundController &instance();

signals:

private:
    explicit SoundController(QObject *parent = nullptr);

    void playOtpSound();

    QSoundEffect *m_otpSound = nullptr;
};
