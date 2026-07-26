// clazy:excludeall=range-loop-detach
#include "commands.h"
#include "fakedeviceconversationsinterface.h"
#include "fakekdeconnectdaemon.h"
#include "fakekdeconnectbattery.h"
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

static bool parseUint(const QString &raw, int &out)
{
    bool ok = false;
    out = raw.toInt(&ok);
    if (!ok || out < 0) {
        qWarning() << raw << "is not valid (expected non-negative integer)";
        return false;
    }
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
        numThreadsToRead = numThreadsToReadAsString.toInt(&ok);
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

void cmdList(const QStringList &)
{
    // Access daemon's device vector is private; we can expose a public API or use devices() to list ids.
    auto &configs = g_daemon->getDeviceConfigs();
    for (int i = 0; i < configs.size(); ++i) {
        qInfo().noquote() << QString("%1%2: %3").arg(i == g_deviceIndex ? ">" : " ").arg(i).arg(configs.at(i)->name);
    }
}

void cmdSelect(const QString &idxStr)
{
    int newDeviceIndex;
    if (!parseUint(idxStr, newDeviceIndex)) {
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

    const QString body = args.mid(1).join(' ');

    auto *device = g_daemon->getDeviceConfigs()[g_deviceIndex].get();
    auto message = device->makeMessage(phoneNumbers, body, DeviceConfig::Incoming, {});
    auto *conversationInterface = g_daemon->getConversationsInterface(device->id);

    conversationInterface->simulateIncomingMessage(message);
}


template<typename Setter, typename Getter>
static void applyInterval(const QStringList &args,
                          const char *label,
                          Setter setter,
                          Getter getter)
{
    auto *device = g_daemon->getDeviceConfigs()[g_deviceIndex].get();
    auto *iface  = g_daemon->getConversationsInterface(device->id);

    if (args.isEmpty()) {
        const int val = (iface->*getter)();
        qInfo() << label << "is" << val << "ms";
        return;
    }

    int val = 0;
    if (!parseUint(args[0], val))
        return;

    (iface->*setter)(val);
    qInfo() << label << "set to" << val << "ms";
}

void cmdInterval(const QStringList &args)
{
    applyInterval(args,
                  "Normal traffic interval",
                  &FakeDeviceConversationsInterface::setInterval,
                  &FakeDeviceConversationsInterface::interval);
}

void cmdSendInterval(const QStringList &args)
{
    applyInterval(args,
                  "Outgoing text interval",
                  &FakeDeviceConversationsInterface::setSendInterval,
                  &FakeDeviceConversationsInterface::sendInterval);
}

void cmdAttachmentInterval(const QStringList &args)
{
    applyInterval(args,
                  "Attachment interval",
                  &FakeDeviceConversationsInterface::setAttachmentInterval,
                  &FakeDeviceConversationsInterface::attachmentInterval);
}

void cmdCharge(const QString &rawLevel)
{
    int level;
    if (!parseUint(rawLevel, level)) {
        return;
    }

    auto *device = g_daemon->getDeviceConfigs()[g_deviceIndex].get();
    auto *iface  = g_daemon->getBatteryInterface(device->id);
    iface->setLevel(level);
}

void cmdCharging(double rate)
{
    auto *device = g_daemon->getDeviceConfigs()[g_deviceIndex].get();
    auto *iface  = g_daemon->getBatteryInterface(device->id);
    iface->setChargeRate(rate);
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
    { "list",            0, 0, cmdList,
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

    { "interval",        0, 1, cmdInterval,
     "Set normal traffic interval",
     "interval <ms>" },

    { "sendinterval",    0, 1, cmdSendInterval,
     "Set outgoing text interval",
     "sendinterval <ms>" },

    { "attachmentinterval", 0, 1, cmdAttachmentInterval,
     "Set attachment processing interval",
     "attachmentinterval <ms>" },

    { "charge",          1, 1, [](const QStringList &a){ cmdCharge(a[0]); },
     "Set the charge level of the battery of the current device",
     "charge <0..100>" },

    { "charging",        0, 0, [](const QStringList &a){ cmdCharging(1); },
     "Sets the phone to charge at 1%/minute",
     "charging" },

    { "discharging",        0, 0, [](const QStringList &a){ cmdCharging(-0.25); },
     "Sets the phone to discharge at 1% every 4 minutes",
     "discharging" },

    { "help",            0, 1, cmdHelp,
     "Show help for all commands or one command",
     "help [command]" },

    { "exit",            0, 0, [](const QStringList &){ QCoreApplication::quit(); },
     "Exit the shell",
     "exit" },
};

static const CommandSpec* findCommand(const QString &cmd)
{
    for (const auto &c : g_commands)
        if (c.name == cmd)
            return &c;
    return nullptr;
}

void cmdHelp(const QStringList &args)
{
    if (args.isEmpty()) {
        qInfo().noquote() << "Available commands:";
        for (const auto &c : g_commands)
            qInfo().noquote() << "  " << c.name << " - " << c.description;
        return;
    }

    const CommandSpec *spec = findCommand(args[0].toLower());
    if (!spec) {
        qWarning() << "Unknown command:" << args[0];
        return;
    }

    qInfo().noquote() << spec->name << ": " << spec->description;
    qInfo().noquote() << "Usage: " << spec->usage;
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
        return;
    }

    const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    const QString cmd = parts[0].toLower();
    const QStringList args = parts.mid(1);

    // lookup
    const CommandSpec *spec = findCommand(cmd);
    if (!spec) {
        qWarning() << "Unknown command:" << cmd;
        return;
    }

    // argument count check
    const int argc = args.size();
    if (argc < spec->minArgs || (spec->maxArgs != -1 && argc > spec->maxArgs)) {
        qWarning().noquote() << "Usage: " << spec->usage;
        return;
    }

    // dispatch
    spec->fn(args);
}


} // anonymous namespace

namespace Commands {

void startInteractiveShell(FakeKdeConnectDaemon *daemon)
{
    if (!daemon) return;
    g_daemon = daemon;

    qInfo() << "Fake daemon started - enter 'help' for a list of commands.";
    if (!g_notifier) {
        g_notifier = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read, daemon);
        QObject::connect(g_notifier, &QSocketNotifier::activated, daemon, [](int){ handleCommand(); });
    }
}

} // namespace Commands
