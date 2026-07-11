#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QObject>
#include "kdeconnect_interfaces/conversationmessage.h"

class Attachment;

class AttachmentListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(QList<Attachment> attachments READ attachments WRITE setAttachments)

public:
    enum Roles {
        ExtensionRole = Qt::UserRole + 1,
        SizeRole,
        IndexRole
    };

    struct Item {
        QString extension;
        QString base64EncodedFile;
    };

    explicit AttachmentListModel(QObject *parent = nullptr);

    QList<Attachment> attachments() const { return m_attachments; }
    void setAttachments(const QList<Attachment> &list);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void saveToPath(int index, const QString &path);

private:
    Item convertAttachment(const Attachment &a) const;

private:
    QList<Attachment> m_attachments;
    QVector<Item> m_items;
};
