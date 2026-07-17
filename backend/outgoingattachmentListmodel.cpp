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

OutgoingAttachmentListModel::Item OutgoingAttachmentListModel::makeItem(const QUrl &fileUri) const
{
    QString localPath = fileUri.toLocalFile();
    QMimeDatabase db;
    QMimeType mt = db.mimeTypeForFile(localPath, QMimeDatabase::MatchContent);

    Item item;
    item.fileUri = fileUri;
    item.filename = QFileInfo(localPath).fileName();
    item.isImage = mt.name().startsWith("image/");
    return item;
}

void OutgoingAttachmentListModel::add(const QUrl &fileUri) {
    if (!fileUri.isValid() || !fileUri.isLocalFile()) {
        qWarning() << Q_FUNC_INFO << "bad fileUri:" << fileUri;
        return;
    }

    bool wasEmpty = this->isEmpty();
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.push_back(makeItem(fileUri));
    endInsertRows();
    if (wasEmpty) {
        emit isEmptyChanged();
    }
    emit allChanged();
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
    emit allChanged();
}

QVector<QUrl> OutgoingAttachmentListModel::all() const
{
    QVector<QUrl> result;
    result.reserve(m_items.count());
    for (const auto& item: m_items) {
        result.append(item.fileUri);
    }
    return result;
}

void OutgoingAttachmentListModel::setAll(const QVector<QUrl> &all)
{
    if (all.isEmpty() && m_items.isEmpty())
        return; // Short-circuit a common no-op condition:

    bool wasEmpty = isEmpty();
    beginResetModel();
    m_items.clear();

    QMimeDatabase db;

    for (const QUrl &fileUri : all) {
        if (!fileUri.isValid() || !fileUri.isLocalFile()) {
            qWarning() << Q_FUNC_INFO << "bad fileUri:" << fileUri;
            continue;
        }

        m_items.push_back(makeItem(fileUri));
    }

    endResetModel();

    if (wasEmpty != isEmpty())
        emit isEmptyChanged();
    emit allChanged();
}