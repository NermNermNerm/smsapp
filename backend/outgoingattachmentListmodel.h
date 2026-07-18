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
    Q_PROPERTY(bool isOversized READ isOversized NOTIFY isOversizedChanged FINAL)
    Q_PROPERTY(bool isAbleToDownscale READ isAbleToDownscale NOTIFY isAbleToDownscaleChanged FINAL)
    Q_PROPERTY(bool isDownscaling READ isDownscaling WRITE setIsDownscaling NOTIFY isDownscalingChanged FINAL)
    // isBlockingSend <==> isOversized && (!isAbleToDownscale || !isDownscaling)

public:
    enum Roles {
        FileUriRole = Qt::UserRole + 1,
        FilenameRole,
        IsImageRole,
    };

    struct Item {
        QUrl fileUri;          // "file:///home/steve/foo.png"
        QString filename;         // derived from fileUri - it's just the basename, e.g. 'foo'
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

    bool isOversized() const { return m_isOversized; }
    bool isAbleToDownscale() const { return m_isAbleToDownscale; }
    bool isDownscaling() const { return m_isDownscaling; }

    void setIsDownscaling(bool isDownscaling);

signals:
    void isEmptyChanged();
    void allChanged();
    void isOversizedChanged();
    void isAbleToDownscaleChanged();
    void isDownscalingChanged();

public slots:
    void add(const QUrl &fileUri);
    void remove(int index);

private:
    Item makeItem(const QUrl &fileUri) const;
    void checkSizeLimit();

    QVector<Item> m_items;
    bool m_isOversized = false;
    bool m_isAbleToDownscale = false;
    bool m_isDownscaling = false;
};
