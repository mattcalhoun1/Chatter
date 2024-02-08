#include "RTClockBase.h"

//-D_RENESAS_RA_
// ARDUINO_UNOR4_WIFI

#ifdef ARDUINO_UNOR4_WIFI

#include <DS3231.h>
#include "RTC.h"

#ifndef R4RTCLOCK_H
#define R4RTCLOCK_H

class R4RtClock : public RTClockBase {
    public:
        R4RtClock();
        bool isFunctioning ();

    protected:
        bool readLatestTime ();

        int getYear();
        int getMonth();
        int getDate();
        int getSecond();
        int getMinute();
        int getHour();

        RTCTime rtct; // time wrapper , part of rensas r4 rtc
};

#endif // r4rtclock

#endif // board check