#ifndef SMSMESSAGEITEMLIST_H
#define SMSMESSAGEITEMLIST_H

#include <QAbstractListModel>
#include <QObject>

class SmsMessageItem;
class ConversationMessage;

class SmsMessageItemList : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit SmsMessageItemList(QObject *parent = nullptr);

    enum Roles {
        ObjectRole = Qt::UserRole + 1
    };

    // Required overrides
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addMessageItem(const ConversationMessage &message);
    const SmsMessageItem &first() const {
        return *(m_list[0]);
    }

private:
    QVector<SmsMessageItem*> m_list;
};

#endif // SMSMESSAGEITEMLIST_H
