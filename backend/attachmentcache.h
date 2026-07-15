#pragma once
#include <QString>
#include "kdeconnect_interfaces/conversationmessage.h"

class AttachmentCache
{

public:
    explicit AttachmentCache(const QString &deviceId);

    /** @brief If the given attachment is in the cache, this will return the full
      * path to the file.  If it is not in the cache already, it will return "". */
    QString tryGetCachedAttachment(const Attachment &attachment) const;

    /** @brief Stores the file in the cache. */
    QString emplaceFile(const Attachment &attachment, const QString &sourcePath);

private:
    QString getPathForAttachment(const Attachment &attachment) const;

    const QString m_cacheDir;
    const QString m_copyDir;
};
