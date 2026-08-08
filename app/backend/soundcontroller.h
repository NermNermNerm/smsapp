#pragma once

class QSoundEffect;

class SoundController : public QObject
{
    Q_OBJECT
public:
    static SoundController &instance();

public slots:
    void playOtpSound();

signals:

private:
    explicit SoundController(QObject *parent = nullptr);


    QSoundEffect *m_otpSound = nullptr;
};
