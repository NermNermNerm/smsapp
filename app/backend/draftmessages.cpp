#include "draftmessages.h"
#include "devicestatus.h"

DraftMessages::DraftMessages(QObject *parent)
    : QObject(parent)
{
    QObject::connect(&DeviceStatus::instance(), &DeviceStatus::handlerChanged, this, [this]() {
        // there's no point in sending out notifications, as changing the handler will reset everything
        m_draftAttachments.clear();
        m_draftTexts.clear();
    });
}

DraftMessages &DraftMessages::instance()
{
    static auto *inst = new DraftMessages();
    return *inst;
}