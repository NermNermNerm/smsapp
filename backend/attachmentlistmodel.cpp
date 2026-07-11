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
    case ExtensionRole:
        return item.extension;
    case SizeRole:
        return item.base64EncodedFile.length() * 3 / 4;
    case IndexRole:
        return index.row();
    }

    return {};
}

QHash<int, QByteArray> AttachmentListModel::roleNames() const
{
    return {
        {ExtensionRole, "extension"},
        {SizeRole, "size"},
        {IndexRole, "index"}
    };
}

void AttachmentListModel::download(int index)
{
    if (index < 0 || index >= m_items.size())
        return;

    const Item &item = m_items[index];

    // Decode base64
    const QByteArray decoded = QByteArray::fromBase64(item.base64EncodedFile.toUtf8());

    // Default filename suggestion
    const QString suggested =
        QStringLiteral("attachment.%1").arg(item.extension);

    // Native file-save dialog
    const QString path = QFileDialog::getSaveFileName(
        nullptr,
        tr("Save Attachment"),
        suggested,
        tr("All Files (*.*)")
        );

    if (path.isEmpty())
        return; // user cancelled

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to save attachment:" << path;
        return;
    }

    file.write(decoded);
    file.close();
}

