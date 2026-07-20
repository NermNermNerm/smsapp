#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTextStream>

#include <KPeople/PersonsModel>
#include <KPeople/PersonData>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    KPeople::PersonsModel model;
    model.rowCount(); // force load

    QJsonArray peopleArray;

    for (int row = 0; row < model.rowCount(); ++row) {
        QModelIndex idx = model.index(row, 0);
        QString uri = model.data(idx, KPeople::PersonsModel::PersonUriRole).toString();
        if (uri.isEmpty())
            continue;

        KPeople::PersonData person(uri);

        QJsonObject entry;
        entry.insert(QStringLiteral("name"), person.name());

        // Emails
        QJsonArray emails;
        for (const QString &email : person.allEmails()) {
            emails.append(email);
        }
        entry.insert(QStringLiteral("emails"), emails);

        // Raw phone numbers (NO normalization)
        QVariantList rawPhones =
            person.contactCustomProperty(QStringLiteral("all-phoneNumber")).toList();

        QJsonArray phones;
        for (const QVariant &v : rawPhones) {
            phones.append(v.toString());
        }
        entry.insert(QStringLiteral("phones"), phones);

        peopleArray.append(entry);
    }

    QJsonObject root;
    root.insert(QStringLiteral("kpeopledata"), peopleArray);

    QJsonDocument doc(root);
    QTextStream(stdout) << doc.toJson(QJsonDocument::Compact) << "\n";

    return 0;
}
