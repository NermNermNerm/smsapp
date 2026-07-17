#pragma once
#include <QAbstractListModel>
#include <QMimeDatabase>
#include <QFileInfo>
#include <QImage>
#include <QBuffer>
#include <QUrl>

class OutgoingAttachmentListModel : public QAbstractListModel {
    Q_OBJECT

    Q_PROPERTY(bool isEmpty READ isEmpty NOTIFY isEmptyChanged FINAL)
    Q_PROPERTY(QVector<QUrl> all READ all WRITE setAll NOTIFY allChanged FINAL)

public:
    enum Roles {
        FileUriRole = Qt::UserRole + 1,
        FilenameRole,
        IsImageRole,
    };

    struct Item {
        QUrl fileUri;          // "file:///home/steve/foo.png"
        QString filename;         // derived from fileUri
        bool isImage;             // mimeType.startsWith("image/")
    };

    explicit OutgoingAttachmentListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override {
        return {
            { FileUriRole, "fileUri" },
            { FilenameRole, "filename" },
            { IsImageRole, "isImage" },
        };
    }

    bool isEmpty() const { return m_items.isEmpty(); }
    QVector<QUrl> all() const;
    void setAll(const QVector<QUrl> &all);

signals:
    void isEmptyChanged();
    void allChanged();

public slots:
    void add(const QUrl &fileUri);
    void remove(int index);

private:
    Item makeItem(const QUrl &fileUri) const;

    QVector<Item> m_items;
};
