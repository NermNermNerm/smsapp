#include "installer.h"
#include <filesystem>
#include <fstream>

namespace Installer {

static std::string appLocalPath() {
    // Typically: ~/.local/share
    const QString base =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);

    return (base + "/" + "appsmsapp").toStdString();
}

static std::string iconPath() {
    return appLocalPath() + "/start-menu-icon.png";
}

static void ensureStartMenuIcon()
{
    std::string icon = iconPath();
    if (std::filesystem::exists(icon))
        return;

    std::string appLocal = appLocalPath();
    std::filesystem::create_directories(appLocal);

    // qDebug() << "Root:" << QDir(":/").entryList();
    // qDebug() << "Resources:" << QDir(":/resources").entryList();

    QFile in(":/resources/start-menu-icon.png");
    if (!in.open(QIODevice::ReadOnly)) {
        Q_ASSERT(false);
        return;
    }
    QByteArray data = in.readAll();
    in.close();
    std::filesystem::path p(icon);
    std::ofstream(p, std::ios::binary | std::ios::trunc)
        .write(data.constData(), data.size());
}

void ensureInstalled(char *argv[])
{
    ensureStartMenuIcon();

    std::string appsDir =
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation).toStdString();
    // Typically: ~/.local/share/applications

    std::filesystem::create_directories(appsDir);
    std::string desktopFile = appsDir + "/smsapp.desktop";
    if (std::filesystem::exists(desktopFile))
        return;

    std::string exePath = QFileInfo(QCoreApplication::applicationFilePath()).absoluteFilePath().toStdString();
    std::ofstream(desktopFile, std::ios::trunc) <<
        "[Desktop Entry]\n"
        "Type=Application\n"
        "Name=SMS Mirror\n"
        "Exec=" << exePath << "\n"
        "Icon=" << iconPath() << "\n"
        "Terminal=false\n";

    execve(exePath.c_str(), const_cast<char **>(argv), environ);
}

}
