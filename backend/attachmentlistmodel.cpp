#include "attachmentlistmodel.h"
#include "kdeconnect_interfaces/conversationmessage.h"
#include "mimetypes.h"
#include <QStandardPaths>
#include <QFile>
#include <QUrl>
#include "messageshandler.h"
#include "dbus.h"
#include "kdeconnect_proxy.h"

extern const std::unordered_map<QString, QString> g_mimeToExtension;

AttachmentListModel::AttachmentListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void AttachmentListModel::setAttachments(const QList<Attachment> &list)
{
    beginResetModel();
    m_attachments = list;
    m_items.clear();
    m_items.reserve(list.size());

    for (const Attachment &a : list) {
        Item item;
        item.attachment = a;
        item.extension = getExtensionForMimeType(a.mimeType());
        item.mimeType = a.mimeType();
        // TODO: See if the attachment is in the cache, if so populate item.fileUri;
        m_items.push_back(item);
    }
    endResetModel();
}

void AttachmentListModel::setMessagesHandler(MessagesHandler *messagesHandler)
{
    if (m_messagesHandler == messagesHandler) {
        return;
    }

    m_messagesHandler = messagesHandler;
    if (m_messagesHandler) {
        const QString &deviceId = m_messagesHandler->deviceID();
        const auto &connections = dbus::conversations(deviceId);
        connect(&connections, &org::kde::kdeconnect::conversations::attachmentReceived, this, &AttachmentListModel::onAttachmentReceived);
    }
}

int AttachmentListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant AttachmentListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const Item &item = m_items.at(index.row());

    switch (role) {
    case MimeTypeRole:
        return item.mimeType;
    case ExtensionRole:
        return item.extension;
    case IndexRole:
        return index.row();
    case FileUriRole:
        return item.fileUri.toString();
    case ThumbnailRole:
        return item.attachment.base64EncodedFile();
    }

    return {};
}

QHash<int, QByteArray> AttachmentListModel::roleNames() const
{
    return {
        {MimeTypeRole, "mimeType"},
        {ExtensionRole, "extension"},
        {IndexRole, "index"},
        {FileUriRole, "fileUri"},
        {ThumbnailRole, "thumbnail"}
    };
}

void AttachmentListModel::saveToPath(int index, const QString &path)
{
    // TODO: FIX ME!  (saving the thumbnail doesn't do any real good)
    // TODO: Maybe change the 'download' button for a loading button
    //       that changes into a "open" button when the thing is downloaded
    if (index < 0 || index >= m_items.size())
        return;

    const Item &item = m_items[index];
    const QByteArray decoded = QByteArray::fromBase64(item.attachment.base64EncodedFile().toUtf8());

    QFile file(QUrl(path).toLocalFile());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to save attachment:" << path;
        return;
    }

    file.write(decoded);
    file.close();
}

void AttachmentListModel::open(int index)
{
    // TODO: Even on an image, there should be a swirly button while we wait for the file to arrive
    // TODO: We need to put a tag in the item that says open it when it gets here.
}

void AttachmentListModel::requestFullAttachment(int index)
{
    if (!m_messagesHandler) {
        qWarning() << Q_FUNC_INFO << "messagesHandler property not set";
        return;
    }

    const Item &item = m_items[index];
    // TODO: Have MessageHandler do this.  It should take a path as an argument
    auto reply = dbus::conversations(m_messagesHandler->deviceID()).requestAttachmentFile(item.attachment.partID(), item.attachment.uniqueIdentifier());

    // synchronously waiting is kinda bad, but it's fairly quick and unlikely to ever be a failure.
    // But if it is, we want to know about it.
    reply.waitForFinished();
    if (reply.isError()){
        qWarning() << Q_FUNC_INFO << "requestAttachmentFile failed: " << reply.error();
    }
}

void AttachmentListModel::onAttachmentReceived(const QString &path,
                                               const QString &uniqueID)
{
    int row = -1;
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].attachment.uniqueIdentifier() == uniqueID) {
            row = i;
            break;
        }
    }

    if (row < 0) {
        qWarning() << Q_FUNC_INFO
                   << "got a message for a file we're not waiting for:"
                   << uniqueID;
        return;
    }

    m_items[row].fileUri = QUrl::fromLocalFile(path);

    // Notify QML that fileUri changed
    emit dataChanged(index(row), index(row), { FileUriRole });
}
