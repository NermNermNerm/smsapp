#pragma once

class Main : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString specifiedDeviceId READ specifiedDeviceId CONSTANT)
    Q_PROPERTY(bool startMinimized READ startMinimized CONSTANT)
public:
    static Main &instance();

    int run(int argc, char *argv[]);

    QString specifiedDeviceId() const { return m_specifiedDevice; }
    bool startMinimized() const { return m_startMinimized; }

private:
    Main() {}

    QString m_specifiedDevice;
    bool m_startMinimized = false;
};
