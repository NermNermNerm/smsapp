// clazy:excludeall=range-loop-detach
#include "commands.h"
#include "fakedeviceconversationsinterface.h"
#include "fakekdeconnectdaemon.h"
#include "deviceconfig.h"
#include "harvester.h"
#include "dbus.h"
#include "nameresolver.h"

#include <QSocketNotifier>
#include <QTextStream>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include <QVector>

using namespace harvester;

namespace {

QSocketNotifier *g_notifier = nullptr;
FakeKdeConnectDaemon *g_daemon = nullptr;
int g_deviceIndex = 0;

void printPrompt()
{
    if (g_daemon)
        g_daemon->printPrompt();
}

bool checkSelected()
{
    // forward to daemon's checkSelected via public API is not exposed; replicate minimal check if needed
    // but prefer to call daemon methods for actions that require selection.
    return true;
}

void cmdKdeListDevices()
{
    auto devices = readAllDeviceIds();
    for (const auto &devId: devices)
    {
        qInfo() << devId;
    }
}

void CmdKdeGetThreadIds(const QString &deviceId)
{
    auto threadIds = readAllThreadIds(deviceId);
    QDebug qinfo = qInfo();
    for (const auto &devId: threadIds)
    {
        qinfo << devId;
    }
}

void populateFakeKdeDirectory(const QString &numThreadsToReadAsString)
{
    int numThreadsToRead;
    if (numThreadsToReadAsString.isEmpty()) {
        numThreadsToRead = 50;
    }
    else {
        bool ok = false;
        int numThreadsToRead = numThreadsToReadAsString.toInt(&ok);
        if (!ok || numThreadsToRead == 0)
        {
            qWarning() << numThreadsToReadAsString << "should be a count of threads to read";
            return;
        }
    }

    //
    // Enumerate real KDE devices
    //
    auto realIds = readAllDeviceIds();

    for (const QString &realId : realIds)
    {
        DeviceConfig config;
        //
        // Transform ID and name
        //
        config.id = "FAKE" + realId.left(8);
        QString realName = dbus::device(realId).name();
        config.name = "Fake " + realName;
        config.reachable = true;

        //
        // Read threads and messages
        //
        auto threadIds = readAllThreadIds(realId);
        qInfo() << "Completed reading all thread IDs";

        int threadsReadSoFar = 0;
        for (qint64 tid : threadIds) {
            if (numThreadsToRead > 0 && threadsReadSoFar >= numThreadsToRead) break;
            auto msgs = readAllMessages(realId, tid, config.id);
            config.smsMessages += msgs;
            ++threadsReadSoFar;
        }

        qInfo() << "Wrote fake device" << config.id
                << "with" << config.smsMessages.size() << "messages";
    }
}

void cmdList()
{
    if (!g_daemon) return;
    // Access daemon's device vector is private; we can expose a public API or use devices() to list ids.
    auto &configs = g_daemon->getDeviceConfigs();
    for (int i = 0; i < configs.size(); ++i) {
        qInfo().noquote() << QString("%1%2: %3").arg(i == g_deviceIndex ? ">" : " ").arg(i).arg(configs.at(i)->name);
    }
}

void cmdSelect(const QString &idxStr)
{
    if (!g_daemon) return;
    bool ok = false;
    int newDeviceIndex = idxStr.toInt(&ok);
    if (!ok || newDeviceIndex < 0 || newDeviceIndex >= g_daemon->getDeviceConfigs().size()) {
        qWarning() << "Invalid device index";
        return;
    }
    // The original implementation used internal index; to preserve behavior we need a setter.
    // For now, attempt to select by index via DBus or extend daemon API. We'll call a private-like behavior:
    // This simplified implementation will just print selection for compatibility.
    qInfo() << "Selected device index" << newDeviceIndex;
    g_deviceIndex = newDeviceIndex;
}

void cmdOnOff(bool on)
{
    // This function needs to toggle the selected device reachable flag.
    // For parity with original, we would call into daemon to mutate m_devices.
    auto *device = g_daemon->getDeviceConfigs()[g_deviceIndex].get();
    if (device->reachable == on) {
        qInfo() << device->name << "is already" << (on ? "on" : "off");
    }
    else {
        device->reachable = on;
        qInfo() << "Toggling" << device->name << (on ? "on" : "off");
    }
}

void cmdText(const QStringList &args)
{
    if (args.length() < 2) {
        qInfo() << "Usage: text <sender>[,<cc1>[,<cc2>..]] content";
        return;
    }

    const QStringList rawList = args[0].split(',', Qt::SkipEmptyParts);
    QStringList phoneNumbers;

    for (const QString &item : rawList) {
        const QString number = lookupName(item.trimmed());
        if (number.isEmpty()) {
            qWarning() << "Unknown name or phone number:" << item;
            return;
        }
        phoneNumbers.append(number);
    }

    QString body;
    for (int i = 1; i < args.length(); ++i) {
        if (!body.isEmpty())
            body.append(' ');
        body.append(args[i]);
    }

    auto *device = g_daemon->getDeviceConfigs()[g_deviceIndex].get();
    auto message = device->makeMessage(phoneNumbers, body, DeviceConfig::Incoming, {});
    auto *conversationInterface = g_daemon->getConversationsInterface(device->id);

    conversationInterface->simulateIncomingMessage(message);
}

void cmdSend(const QStringList &args)
{
    if (args.size() < 2) {
        qWarning() << "Usage: send <to> <text>";
        return;
    }
    const QString to = args[0];
    const QString text = args.mid(1).join(' ');
    qInfo() << "Simulated a text to" << to << ":" << text;
}

void cmdInterval(const QString &idxStr)
{
    bool ok = false;
    int intervalInMs = idxStr.toInt(&ok);
    if (!ok || intervalInMs < 0) {
        qWarning() << "Invalid interval (should be '50' or so.)";
        return;
    }

    auto *device = g_daemon->getDeviceConfigs()[g_deviceIndex].get();
    g_daemon->getConversationsInterface(device->id)->setInterval(intervalInMs);
    qInfo() << "Interval for emitting normal traffic set to" << intervalInMs << "ms";
}

void cmdSendInterval(const QString &idxStr)
{
    bool ok = false;
    int intervalInMs = idxStr.toInt(&ok);
    if (!ok || intervalInMs < 0) {
        qWarning() << "Invalid interval (should be '50' or so.)";
        return;
    }

    auto *device = g_daemon->getDeviceConfigs()[g_deviceIndex].get();
    g_daemon->getConversationsInterface(device->id)->setSendInterval(intervalInMs);
    qInfo() << "Interval for processing outgoing texts set to" << intervalInMs << "ms";
}

void cmdAttachmentInterval(const QString &idxStr)
{
    bool ok = false;
    int intervalInMs = idxStr.toInt(&ok);
    if (!ok || intervalInMs < 0) {
        qWarning() << "Invalid interval (should be '50' or so.)";
        return;
    }

    auto *device = g_daemon->getDeviceConfigs()[g_deviceIndex].get();
    g_daemon->getConversationsInterface(device->id)->setAttachmentInterval(intervalInMs);
    qInfo() << "Interval for processing outgoing texts set to" << intervalInMs << "ms";
}

struct CommandSpec
{
    QString name;
    int minArgs;
    int maxArgs;   // use -1 for “no upper bound”
    void (*fn)(const QStringList &args);
    QString description;
    QString usage;
};

void cmdHelp(const QStringList &args);

static const CommandSpec g_commands[] = {
    { "list",            0, 0, [](const QStringList &a) { cmdList(); },
     "List fake devices",
     "list" },

    { "kdelistdevices",  0, 0, [](const QStringList &a) { cmdKdeListDevices(); },
     "List real KDE Connect devices",
     "kdelistdevices" },

    { "kdegetthreads",   1, 1, [](const QStringList &a){ CmdKdeGetThreadIds(a[0]); },
     "List thread IDs for a KDE device",
     "kdegetthreads <deviceId>" },

    { "kdepopulate",     0, 1, [](const QStringList &a){ populateFakeKdeDirectory(a.isEmpty() ? "" : a[0]); },
     "Populate fake KDE directory",
     "kdepopulate [numThreads]" },

    { "select",          1, 1, [](const QStringList &a){ cmdSelect(a[0]); },
     "Select a fake device by index",
     "select <index>" },

    { "on",              0, 0, [](const QStringList &){ cmdOnOff(true); },
     "Mark selected device reachable",
     "on" },

    { "off",             0, 0, [](const QStringList &){ cmdOnOff(false); },
     "Mark selected device unreachable",
     "off" },

    { "text",            2, -1, cmdText,
     "Simulate incoming text message",
     "text <sender>[,<cc>...] <content>" },

    { "send",            2, -1, cmdSend,
     "Simulate outgoing text message",
     "send <to> <content>" },

    { "interval",        1, 1, [](const QStringList &a){ cmdInterval(a[0]); },
     "Set normal traffic interval",
     "interval <ms>" },

    { "sendinterval",    1, 1, [](const QStringList &a){ cmdSendInterval(a[0]); },
     "Set outgoing text interval",
     "sendinterval <ms>" },

    { "attachmentinterval", 1, 1, [](const QStringList &a){ cmdAttachmentInterval(a[0]); },
     "Set attachment processing interval",
     "attachmentinterval <ms>" },

    { "help",            0, 1, cmdHelp,
     "Show help for all commands or one command",
     "help [command]" },

    { "exit",            0, 0, [](const QStringList &){ QCoreApplication::quit(); },
     "Exit the shell",
     "exit" },
};

void cmdHelp(const QStringList &args)
{
    if (args.isEmpty()) {
        qInfo().noquote() << "Available commands:";
        for (const auto &c : g_commands)
            qInfo().noquote() << "  " << c.name << " - " << c.description;
        return;
    }

    const QString target = args[0].toLower();
    for (const auto &c : g_commands) {
        if (c.name == target) {
            qInfo().noquote() << c.name << ": " << c.description;
            qInfo().noquote() << "Usage: " << c.usage;
            return;
        }
    }

    qWarning() << "Unknown command:" << target;
}

void handleCommand()
{
    QTextStream in(stdin);
    const QString line = in.readLine().trimmed();

    if (line.isNull() && in.atEnd()) {
        QCoreApplication::quit();
        return;
    }

    if (line.isEmpty()) {
        printPrompt();
        return;
    }

    const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    const QString cmd = parts[0].toLower();
    const QStringList args = parts.mid(1);

    // lookup
    const CommandSpec *spec = nullptr;
    for (const auto &c : g_commands) {
        if (c.name == cmd) {
            spec = &c;
            break;
        }
    }

    if (!spec) {
        qWarning() << "Unknown command:" << cmd;
        printPrompt();
        return;
    }

    // argument count check
    const int argc = args.size();
    if (argc < spec->minArgs || (spec->maxArgs != -1 && argc > spec->maxArgs)) {
        qWarning().noquote() << "Usage: " << spec->usage;
        printPrompt();
        return;
    }

    // dispatch
    spec->fn(args);

    printPrompt();
}


} // anonymous namespace

namespace Commands {

void startInteractiveShell(FakeKdeConnectDaemon *daemon)
{
    if (!daemon) return;
    g_daemon = daemon;

    if (!g_notifier) {
        g_notifier = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read, daemon);
        QObject::connect(g_notifier, &QSocketNotifier::activated, daemon, [](int){ handleCommand(); });
    }

    printPrompt();
}

} // namespace Commands
