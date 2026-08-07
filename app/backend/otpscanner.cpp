#include "otpscanner.h"

OtpScanner &OtpScanner::instance()
{
    auto *inst = new OtpScanner();
    return *inst;
}