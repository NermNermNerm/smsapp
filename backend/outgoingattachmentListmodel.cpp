#include <QAbstractListModel>
#include <QMimeDatabase>
#include <QFileInfo>
#include <QImage>
#include <QBuffer>
#include <QUrl>
#include "outgoingattachmentListmodel.h"

OutgoingAttachmentListModel::OutgoingAttachmentListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}


int OutgoingAttachmentListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant OutgoingAttachmentListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const Item &item = m_items.at(index.row());

    switch (role) {
    case FileUriRole:         return item.fileUri;
    case FilenameRole:        return item.filename;
    case IsImageRole:         return item.isImage;
    }

    return {};
}

void OutgoingAttachmentListModel::add(const QUrl &fileUri) {
    if (!fileUri.isValid() || !fileUri.isLocalFile()) {
        qWarning() << Q_FUNC_INFO << "bad fileUri:" << fileUri;
        return;
    }

    QString localPath = fileUri.toLocalFile();
    QMimeDatabase db;
    QMimeType mt = db.mimeTypeForFile(localPath, QMimeDatabase::MatchContent);

    Item item;
    item.fileUri = fileUri;
    item.filename = QFileInfo(localPath).fileName();
    item.isImage = mt.name().startsWith("image/");

    bool wasEmpty = this->isEmpty();
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.push_back(item);
    endInsertRows();
    if (wasEmpty) {
        emit isEmptyChanged();
    }
}

void OutgoingAttachmentListModel::remove(int index) {
    if (index < 0 || index >= m_items.size())
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_items.removeAt(index);
    endRemoveRows();
    if (isEmpty()) {
        emit isEmptyChanged();
    }
}

QVariantList OutgoingAttachmentListModel::getAll() const
{
    QVariantList result;
    for (const auto& item: m_items) {
        result.append(item.fileUri.toString());
    }
    return result;
}
