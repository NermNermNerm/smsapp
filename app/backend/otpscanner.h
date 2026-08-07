#ifndef OTPSCANNER_H
#define OTPSCANNER_H

#include <QObject>

class OtpScanner : public QObject
{
    Q_OBJECT
public:

    static OtpScanner &instance();

signals:

private:
    explicit OtpScanner() {}
};

#endif // OTPSCANNER_H
