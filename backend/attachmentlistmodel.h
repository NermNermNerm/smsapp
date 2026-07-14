#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QObject>
#include <QUrl>
#include "kdeconnect_interfaces/conversationmessage.h"
#include "messageshandler.h"

class Attachment;
class MessagesHandler;

class AttachmentListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QList<Attachment> attachments READ attachments WRITE setAttachments)
    Q_PROPERTY(MessagesHandler *messagesHandler READ messagesHandler WRITE setMessagesHandler)

public:
    enum Roles {
        MimeTypeRole = Qt::UserRole + 1,
        ExtensionRole,
        FileUriRole,
        ThumbnailRole,
        IndexRole,
    };

    struct Item {
        Attachment attachment;
        QString mimeType;
        QString extension;
        QUrl fileUri;
    };

    explicit AttachmentListModel(QObject *parent = nullptr);

    QList<Attachment> attachments() const { return m_attachments; }
    void setAttachments(const QList<Attachment> &list);

    MessagesHandler *messagesHandler() const { return m_messagesHandler; }
    void setMessagesHandler(MessagesHandler *messagesHandler);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void saveToPath(int index, const QString &path);
    void open(int index);
    void requestFullAttachment(int index);

private:
    void onAttachmentReceived(const QString &path, const QString &uniqueID);

    QList<Attachment> m_attachments;
    QVector<Item> m_items;
    MessagesHandler *m_messagesHandler = nullptr;
};
