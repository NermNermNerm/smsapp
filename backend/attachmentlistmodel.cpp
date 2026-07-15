#include "attachmentlistmodel.h"
#include "kdeconnect_interfaces/conversationmessage.h"
#include "mimetypes.h"
#include <QStandardPaths>
#include <QDesktopServices>
#include <QFile>
#include <QUrl>
#include "messageshandler.h"

AttachmentListModel::AttachmentListModel(MessagesHandler *messagesHandler, QObject *parent)
    : QAbstractListModel(parent), m_messagesHandler(messagesHandler)
{
    Q_ASSERT(messagesHandler != nullptr);
    connect(m_messagesHandler, &MessagesHandler::attachmentRecieved, this, &AttachmentListModel::onAttachmentReceived);
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
        QString filePath = m_messagesHandler->tryGetCachedAttachment(a);
        if (filePath == "") {
            filePath = m_messagesHandler->tryFindCompletedDownload(a);
        }
        if (filePath != "")
        {
            item.fileUri = QUrl::fromLocalFile(filePath);
        }
        item.isDownloading = m_messagesHandler->isDownloadUnderway(a);
        m_items.push_back(item);
    }
    endResetModel();
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
        return item.attachment.mimeType();
    case ExtensionRole:
        return item.extension;
    case IndexRole:
        return index.row();
    case FileUriRole:
        return item.fileUri.toString();
    case ThumbnailRole:
        return item.attachment.base64EncodedFile();
    case IsLoadingRole:
        return item.isDownloading;
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
        {ThumbnailRole, "thumbnail"},
        {DownloadLocationRole, "downloadLocation"},
        {IsLoadingRole, "isLoading"}
    };
}

void AttachmentListModel::saveToPath(int row, const QString &path)
{
    if (!m_messagesHandler) {
        qWarning() << Q_FUNC_INFO << "messagesHandler property not set";
        return;
    }

    if (row < 0 || row >= m_items.size()) {
        qWarning() << Q_FUNC_INFO << "index out of range" << row;
        return;
    }

    Item &item = m_items[row];
    Q_ASSERT(!item.isDownloading && !item.isOpening && item.downloadLocation == ""); // UI should prevent this

    item.isDownloading = true;
    item.isOpening = false;
    item.downloadLocation = "";
    emit dataChanged(index(row), index(row), { IsLoadingRole });
    m_messagesHandler->requestAttachment(item.attachment, path);
}

void AttachmentListModel::open(int row)
{
    if (!m_messagesHandler) {
        qWarning() << Q_FUNC_INFO << "messagesHandler property not set";
        return;
    }

    if (row < 0 || row >= m_items.size()) {
        qWarning() << Q_FUNC_INFO << "index out of range" << row;
        return;
    }

    Item &item = m_items[row];

    if (!item.fileUri.isEmpty()) {
        QDesktopServices::openUrl(item.fileUri);
        return;
    }

    Q_ASSERT(!item.isDownloading && !item.isOpening && item.downloadLocation == ""); // UI should prevent this

    item.isDownloading = true;
    item.isOpening = true;
    item.downloadLocation = "";
    emit dataChanged(index(row), index(row), { IsLoadingRole });
    m_messagesHandler->requestAttachment(item.attachment);
}

void AttachmentListModel::requestFullAttachment(int row)
{
    if (!m_messagesHandler) {
        qWarning() << Q_FUNC_INFO << "messagesHandler property not set";
        return;
    }

    if (row < 0 || row >= m_items.size()) {
        qWarning() << Q_FUNC_INFO << "index out of range" << row;
        return;
    }

    Item &item = m_items[row];
    if (!item.isDownloading && item.downloadLocation != "") {
    }

    item.isDownloading = true;
    item.isOpening = false;
    item.downloadLocation = "";
    emit dataChanged(index(row), index(row), { IsLoadingRole });
    m_messagesHandler->requestAttachment(item.attachment);
}

void AttachmentListModel::onAttachmentReceived(const Attachment &attachment, const QString &path, bool isInCache)
{
    int row = -1;
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].attachment.uniqueIdentifier() == attachment.uniqueIdentifier()) {
            row = i;
            break;
        }
    }

    if (row < 0) {
        // not one of ours; not too surprising, there can be multiple attachment lists in-play
        return;
    }

    auto &item = m_items[row];
    if (path == "") {
        // error messages are going to somehow get shunted into the status bar.
        item.isDownloading = false;
        item.isOpening = false;
        item.downloadLocation = "";
        emit dataChanged(index(row), index(row), { FileUriRole });
        emit dataChanged(index(row), index(row), { IsLoadingRole });
        return;
    }

    m_items[row].fileUri = QUrl::fromLocalFile(path);
    item.isDownloading = false;
    emit dataChanged(index(row), index(row), { FileUriRole });
    emit dataChanged(index(row), index(row), { IsLoadingRole });
    if (item.isOpening) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}
