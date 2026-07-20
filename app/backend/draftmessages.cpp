#include "draftmessages.h"
#include "devicestatus.h"

DraftMessages::DraftMessages(DeviceStatus &deviceStatus, QObject *parent)
    : QObject(parent)
{
    QObject::connect(&deviceStatus, &DeviceStatus::handlerChanged, &deviceStatus, [this]() {
        // there's no point in sending out notifications, as changing the handler will reset everything
        m_draftAttachments.clear();
        m_draftTexts.clear();
    });
}