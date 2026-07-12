#include "attachmentlistmodel.h"
#include "kdeconnect_interfaces/conversationmessage.h"
#include "mimetypes.h"
#include <QFileDialog>

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
        m_items.push_back(convertAttachment(a));
    }
    endResetModel();
}

AttachmentListModel::Item AttachmentListModel::convertAttachment(const Attachment &a) const
{
    Item item;
    item.extension = getExtensionForMimeType(a.mimeType());
    item.base64EncodedFile = a.base64EncodedFile();
    item.mimeType = a.mimeType();
    return item;
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
    case Base64Role:
        return item.base64EncodedFile;
    case IndexRole:
        return index.row();
    }

    return {};
}

QHash<int, QByteArray> AttachmentListModel::roleNames() const
{
    return {
        {MimeTypeRole, "mimeType"},
        {ExtensionRole, "extension"},
        {Base64Role, "base64"},
        {IndexRole, "index"}
    };
}

void AttachmentListModel::saveToPath(int index, const QString &path)
{
    if (index < 0 || index >= m_items.size())
        return;

    const Item &item = m_items[index];
    const QByteArray decoded = QByteArray::fromBase64(item.base64EncodedFile.toUtf8());

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

}