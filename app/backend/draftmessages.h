#pragma once
class DeviceStatus;

class DraftMessages : public QObject
{
    Q_OBJECT

public:
    static DraftMessages &instance();

    bool containsDraft(qint64 conversationID) const {
        return m_draftTexts.contains(conversationID) || m_draftAttachments.contains(conversationID);
    }
    QString getDraftText(qint64 conversationID) const {
        return m_draftTexts.value(conversationID);
    }
    QStringList getDraftAttachments(qint64 conversationID) const {
        return m_draftAttachments.value(conversationID);
    }

    void clearDraft(qint64 conversationID) {
        if (m_draftTexts.remove(conversationID)) {
            emit draftTextChanged(conversationID);
        }

        if (m_draftAttachments.remove(conversationID)) {
            emit draftAttachmentsChanged(conversationID);
        }
    }

    void setDraftText(qint64 conversationID, QString text) {
        if (text.isEmpty()) {
            if (m_draftTexts.remove(conversationID)) {
                emit draftTextChanged(conversationID);
            }
        }
        else {
            auto oldText = getDraftText(conversationID);
            m_draftTexts[conversationID] = text;
            if (oldText != text)
                emit draftTextChanged(conversationID);
        }
    }

    void setDraftAttachments(qint64 conversationID, QStringList attachments) {
        if (attachments.isEmpty()) {
            if (m_draftAttachments.remove(conversationID)) {
                emit draftAttachmentsChanged(conversationID);
            }
        }
        else {
            auto oldAttachments = getDraftAttachments(conversationID);
            m_draftAttachments[conversationID] = attachments;
            if (oldAttachments.join('\n') != attachments.join('\n'))
                emit draftAttachmentsChanged(conversationID);
        }
    }

signals:
    void draftTextChanged(qint64 conversationID);
    void draftAttachmentsChanged(qint64 conversationID);

private:
    explicit DraftMessages(QObject *parent = nullptr);

    QHash<qint64, QString> m_draftTexts;
    QHash<qint64, QStringList> m_draftAttachments;
};
