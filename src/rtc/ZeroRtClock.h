#include "RTClockBase.h"
#include <Arduino.h>

#ifdef ARDUINO_SAMD_MKRZERO

#include <DS3231.h>
#include "RTCZero.h"

#ifndef ZERORTCLOCK_H
#define ZERORTCLOCK_H

class ZeroRtClock : public RTClockBase {
    public:
        ZeroRtClock();
        bool isFunctioning ();
        bool syncWithExternalRtc ();

        int getYear();
        int getMonth();
        int getDate();
        int getSecond();
        int getMinute();
        int getHour();

        bool readLatestTime ();
    protected:
        void updateTimeBuffer();

        RTCZero rtc;
        DS3231 ds3231; // The standalone i2c rtc
};

#endif // zerortclock_h

#endif // board check