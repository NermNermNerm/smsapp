#include "soundcontroller.h"
#include "backend/otpscanner.h"
#include <QtMultimedia/QSoundEffect>

SoundController::SoundController(QObject *parent)
    : QObject{parent}
{
    connect(&OtpScanner::instance(), &OtpScanner::otpReceived, this, &SoundController::playOtpSound);
}

void SoundController::playOtpSound()
{
    if (m_otpSound == nullptr) {
        m_otpSound = new QSoundEffect(this);
        m_otpSound->setSource(QUrl("qrc:/resources/otp-copied.wav"));
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
