#include "RTClockBase.h"

#if defined(ARDUINO_SAM_DUE)

#include <DS3231.h>
#include "RTCDue.h"

#ifndef R4RTCLOCK_H
#define R4RTCLOCK_H

class DueRtClock : public RTClockBase {
    public:
        DueRtClock();
        bool isFunctioning ();
        bool syncWithExternalRtc ();
        bool setNewDateTime (const char* yymmddHHMMSS);
        
    protected:
        bool readLatestTime ();

        int getYear();
        int getMonth();
        int getDate();
        int getSecond();
        int getMinute();
        int getHour();

        RTCDue rtcd = RTCDue(XTAL); 
        //RTCDue rtc(RC);
        //RTCDue rtc(XTAL);        
};

#endif // due rt clock

#endif // board check