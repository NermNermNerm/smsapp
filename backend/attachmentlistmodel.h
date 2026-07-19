#include "pch.h"
#pragma once

#include "kdeconnect_interfaces/conversationmessage.h"
#include "messageshandler.h"
#include "devicestatus.h"

class Attachment;
class MessagesHandler;

class AttachmentListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QList<Attachment> attachments READ attachments WRITE setAttachments)

public:
    enum Roles {
        MimeTypeRole = Qt::UserRole + 1,
        ExtensionRole,
        FileUriRole,
        ThumbnailRole,
        IndexRole,
        DownloadLocationRole,
        IsLoadingRole,
        IsExpandedRole,
    };

    struct Item {
        Attachment attachment;
        QString extension;
        QUrl fileUri;
        bool isDownloading = false;
        bool isOpening = false;
        QString downloadLocation;
        bool isExpanded = false;
    };

    explicit AttachmentListModel(MessagesHandler *messageHandler = DeviceStatus::instance()->handler(), QObject *parent = nullptr);

    QList<Attachment> attachments() const { return m_attachments; }
    void setAttachments(const QList<Attachment> &list);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void saveToPath(int index, const QString &path);
    void open(int index);
    void requestFullAttachment(int index);
    void toggleExpanded(int row);

private:
    void onAttachmentReceived(const Attachment &attachment, const QString &path, bool isInCache);

    QList<Attachment> m_attachments;
    QVector<Item> m_items;
    MessagesHandler * const m_messagesHandler;
};
