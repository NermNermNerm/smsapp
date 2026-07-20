// clazy:excludeall=range-loop-detach
#include "deviceconfig.h"
#include <QDir>
#include "kdeconnect_interfaces/conversationmessage_ext.h"
#include <QMimeDatabase>
#include <QImage>
#include <QBuffer>

std::vector<std::unique_ptr<DeviceConfig>> DeviceConfig::load()
{
    std::vector<std::unique_ptr<DeviceConfig>> devices;

    QString baseDir = QDir::homePath() + "/fakekde";
    QDir dir(baseDir);

    if (!dir.exists()) {
        qWarning() << "Fake KDE directory does not exist:" << baseDir;
        return devices;
    }

    // Only look for "<deviceid>.json" (not "_sms.json")
    QStringList files = dir.entryList(QStringList() << "*.json", QDir::Files);

    for (const QString &file : files)
    {
        if (file.endsWith("_sms.json"))
            continue;

        QString deviceJsonPath = dir.filePath(file);

        //
        // Load <deviceid>.json (required)
        //
        QFile deviceFile(deviceJsonPath);
        if (!deviceFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Cannot open" << deviceJsonPath;
            return devices;
        }

        QJsonDocument deviceDoc = QJsonDocument::fromJson(deviceFile.readAll());
        deviceFile.close();

        if (!deviceDoc.isObject()) {
            qWarning() << "Invalid JSON in" << deviceJsonPath;
            return devices;
        }

        QJsonObject deviceObj = deviceDoc.object();

        auto config = std::make_unique<DeviceConfig>();
        config->id = deviceObj.value("id").toString();
        config->name = deviceObj.value("name").toString();
        config->reachable = deviceObj.value("reachable").toBool();

        //
        // Load <deviceid>_sms.json (optional)
        //
        QString smsJsonPath =
            dir.filePath(file.left(file.size() - 5) + "_sms.json");

        QFile smsFile(smsJsonPath);
        if (smsFile.exists() && smsFile.open(QIODevice::ReadOnly)) {

            QJsonDocument smsDoc =
                QJsonDocument::fromJson(smsFile.readAll());
            smsFile.close();

            if (smsDoc.isObject()) {
                QJsonArray arr =
                    smsDoc.object().value("messages").toArray();

                config->smsMessages.reserve(arr.size());

                for (const QJsonValue &v : arr) {
                    if (v.isObject()) {
                        config->smsMessages.emplaceBack(messageFromJson(v.toObject()));
                    }
                }
            }
        }

        devices.push_back(std::move(config));
    }

    return devices;
}



void DeviceConfig::save()
{
    QString baseDir = QDir::homePath() + "/fakekde";
    QDir dir(baseDir);

    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "Cannot create fakekde directory:" << baseDir;
            return;
        }
    }

    //
    // Write <fakeid>.json (metadata)
    //
    {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["reachable"] = true;

        QString path = baseDir + "/" + id + ".json";
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QJsonDocument doc(obj);
            f.write(doc.toJson(QJsonDocument::Indented));
            f.close();
        } else {
            qWarning() << "Cannot write" << path;
        }
    }

    //
    // Write <fakeid>_sms.json (bulk messages)
    //
    {
        QJsonArray arr;
        for (const auto &msg : smsMessages) {
            arr.append(toJson(msg));
        }

        QJsonObject root;
        root["messages"] = arr;

        QString path = baseDir + "/" + id + "_sms.json";
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QJsonDocument doc(root);
            f.write(doc.toJson(QJsonDocument::Indented));
            f.close();
        } else {
            qWarning() << "Cannot write" << path;
        }
    }
}


ConversationMessage DeviceConfig::makeMessage(const QStringList &addresses, const QString &body, Direction direction, const QVariantList &attachmentUrls)
{
    // ------------------------------------------------------------
    // 1. Android-style loose phone-number match (suffix compare)
    // ------------------------------------------------------------
    auto sameNumber = [](const QString &a, const QString &b) {
        QString da, db;

        for (QChar c : a) if (c.isDigit()) da.append(c);
        for (QChar c : b) if (c.isDigit()) db.append(c);

        if (da.isEmpty() || db.isEmpty())
            return false;

        const int N = 7;
        if (da.length() > N) da = da.right(N);
        if (db.length() > N) db = db.right(N);

        return da == db;
    };

    auto sameAddressSet = [&](const QStringList &lhs,
                              const QList<ConversationAddress> &rhs) {
        if (lhs.size() != rhs.size())
            return false;

        for (const QString &a : lhs) {
            bool found = false;
            for (const auto &b : rhs) {
                if (sameNumber(a, b.address())) {
                    found = true;
                    break;
                }
            }
            if (!found)
                return false;
        }
        return true;
    };

    // ------------------------------------------------------------
    // 2. Try to find an existing threadID
    // ------------------------------------------------------------
    qint64 threadID = -1;

    for (const ConversationMessage &m : smsMessages) {
        if (sameAddressSet(addresses, m.addresses())) {
            threadID = m.threadID();
            break;
        }
    }

    // ------------------------------------------------------------
    // 3. If no thread found, generate a new qint64 threadID
    // ------------------------------------------------------------
    if (threadID == -1) {
        qint64 nextThread = 10000;
        for (const ConversationMessage &m : smsMessages) {
            if (m.threadID() >= nextThread)
                nextThread = m.threadID() + 1;
        }
        threadID = nextThread;
    }

    // ------------------------------------------------------------
    // 4. Generate a unique uID starting at 10000
    // ------------------------------------------------------------
    int nextUID = 10000;
    for (const ConversationMessage &m : smsMessages) {
        if (m.uID() >= nextUID)
            nextUID = m.uID() + 1;
    }

    QList<ConversationAddress> conversationAddresses;
    conversationAddresses.reserve(addresses.size());
    for (const QString &addr: addresses) {
        conversationAddresses.emplaceBack(ConversationAddress(addr));
    }

    // ------------------------------------------------------------
    // 5. Construct the message
    // ------------------------------------------------------------
    ConversationMessage msg(
        ConversationMessage::EventTextMessage /* eventField */,
        body,
        conversationAddresses,
        QDateTime::currentMSecsSinceEpoch(),
        (direction == Incoming ? ConversationMessage::MessageTypeInbox : ConversationMessage::MessageTypeSent), /* type  */
        (direction == Incoming ? 0 : 1), /* read */
        threadID,
        nextUID,
        2, /* subID */
        makeAttachments(attachmentUrls));

    return msg;
}

ConversationMessage DeviceConfig::makeMessage(qint64 conversationID, const QString &body, Direction direction, const QVariantList &attachmentPaths)
{
    ConversationMessage *latestInThread = nullptr;
    for (auto &message: smsMessages) {
        if (message.threadID() == conversationID && (latestInThread == nullptr || latestInThread->date() < message.date())) {
            latestInThread = &message;
        }
    }

    if (latestInThread == nullptr) {
        qWarning() << "conversationID given to DeviceCOnfig::makeMessage (" << conversationID << ") was invalid?!";
        return {};
    }

    int nextUID = 10000;
    for (const ConversationMessage &m : smsMessages) {
        if (m.uID() >= nextUID)
            nextUID = m.uID() + 1;
    }

    ConversationMessage msg(
        ConversationMessage::EventTextMessage /* eventField */,
        body,
        latestInThread->addresses(),
        QDateTime::currentMSecsSinceEpoch(),
        (direction == Incoming ? ConversationMessage::MessageTypeInbox : ConversationMessage::MessageTypeSent), /* type  */
        (direction == Incoming ? 0 : 1), /* read */
        conversationID,
        nextUID,
        2, /* subID */
        makeAttachments(attachmentPaths));

    return msg;
}

QList<Attachment> DeviceConfig::makeAttachments(const QVariantList &attachmentPaths)
{
    int partId = 10000;
    for (const ConversationMessage &m : smsMessages) {
        for (const Attachment &a : m.attachments()) {
            if (a.partID() >= partId) {
                partId = a.partID()+1;
            }
        }
    }

    QList<Attachment> result;
    result.reserve(attachmentPaths.size());
    for (const auto &urlVariant: attachmentPaths)
    {
        QString localPath = urlVariant.toString();
        if (!QFile::exists(localPath)) {
            qWarning() << "Got an attachment filename that's not an extant file: " << localPath;
            continue;
        }

        QString uniqueId = QString::number(partId) + "_" + QFileInfo(localPath).completeBaseName();
        QString fakeKdeCacheLocation = QDir::homePath() + "/fakekde/" + id + "/" + uniqueId;
        if (!QFile::copy(localPath, fakeKdeCacheLocation)) {
            qWarning() << "Couldn't copy attachment: " << localPath << "to" << fakeKdeCacheLocation;
            continue;
        }

        QMimeDatabase db;
        QMimeType mt = db.mimeTypeForFile(localPath, QMimeDatabase::MatchContent);
        QString mimeType = mt.name();
        QString base64Thumb;
        if (mt.name().startsWith("image/")) {
            QImage img(localPath);
            if (!img.isNull()) {
                QImage thumb = img.scaled(100, 100,
                                          Qt::KeepAspectRatio,
                                          Qt::SmoothTransformation);

                QByteArray ba;
                QBuffer buf(&ba);
                buf.open(QIODevice::WriteOnly);
                thumb.save(&buf, "JPEG");
                base64Thumb = ba.toBase64();
            }
        }

        result.emplaceBack(Attachment(partId, mimeType, base64Thumb, uniqueId));
        ++partId;
    }

    return result;
}