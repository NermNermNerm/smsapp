#include "soundcontroller.h"
#include "backend/otpscanner.h"
#include <QtMultimedia/QSoundEffect>

SoundController::SoundController(QObject *parent)
    : QObject{parent}
{
    connect(&OtpScanner::instance(), &OtpScanner::otpReceived, this, [this]() { });
}

void SoundController::playOtpSound()
{
    if (m_otpSound == nullptr) {
        m_otpSound = new QSoundEffect(this);
        m_otpSound->setSource(QUrl("qrc:/otp-copied.mp3"));
        m_otpSound->setLoopCount(1);
        m_otpSound->setVolume(0.8f);
    }
    m_otpSound->play();
}

SoundController &SoundController::instance()
{
    static auto *inst = new SoundController();
    return *inst;
}
