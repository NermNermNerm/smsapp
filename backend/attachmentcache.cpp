#include "attachmentcache.h"
#include "mimetypes.h"

static QString getCacheDir(const QString &deviceId)
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) {
        qWarning() << "CacheManager::load: no application data location is available?!";
        base = "/tmp";
    }

    base = base + "/" + deviceId;
    return base;
}

AttachmentCache::AttachmentCache(const QString &deviceId)
    : m_cacheDir(getCacheDir(deviceId)), m_copyDir(m_cacheDir+"/copy")
{
    QDir().mkpath(m_copyDir); // makes m_cacheDir along the way
}

QString AttachmentCache::tryGetCachedAttachment(const Attachment &attachment) const
{
    auto filename = getPathForAttachment(attachment);
    return QFile::exists(filename) ? filename : "";
}

QString AttachmentCache::emplaceFile(const Attachment &attachment, const QString &sourcePath)
{
    const QString finalPath = getPathForAttachment(attachment);
    const QString tempPath = m_copyDir + "/" + QFileInfo(finalPath).fileName();

    if (QFile::exists(tempPath)) QFile::remove(tempPath);
    if (!QFile::copy(sourcePath, tempPath)) {
        qWarning() << "AttachmentCache::emplaceFile: failed to copy" << sourcePath << "to" << tempPath;
        return QString();
    }

    if (QFile::exists(finalPath)) QFile::remove(finalPath);
    if (!QFile::rename(tempPath, finalPath)) {
        qWarning() << "AttachmentCache::emplaceFile: failed to move" << tempPath << "to" << finalPath;
        QFile::remove(tempPath);
        return QString();
    }

    return finalPath;
}


QString AttachmentCache::getPathForAttachment(const Attachment &attachment) const
{
    const QString filename = attachment.uniqueIdentifier();
    const QString extension = getExtensionForMimeType(attachment.mimeType());

    // If unique already ends with the correct extension, use it directly
    if (filename.endsWith("." + extension)) {
        return m_cacheDir + "/" + filename;
    }

    // Otherwise append the extension
    return m_cacheDir + "/" + filename + "." + extension;
}
