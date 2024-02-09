#include <TimeLib.h>
#include <Arduino.h>
#include <cstdio>

#ifndef RTCLOCKBASE_H
#define RTCLOCKBASE_H

class RTClockBase {
    public:
        virtual bool isFunctioning () = 0;
        const char* getSortableTime ();
        const char* getSortableTimePlusSeconds (int seconds);
        const char* getViewableTime ();

        virtual int getYear() = 0;
        virtual int getMonth() = 0;
        virtual int getDate() = 0;
        virtual int getSecond() = 0;
        virtual int getMinute() = 0;
        virtual int getHour() = 0;

        virtual bool readLatestTime () = 0;

        virtual bool syncWithExternalRtc () = 0;

    protected:
        const char* sortableTimeFormat = "%02d%02d%02d%02d%02d%02d"; // YYMMDDhhmmss
        const char* viewableTimeFormat = "%02d/%02d/%02d %02d:%02d:%02d"; // MM/DD/YYYY hh:mm:ss

        char sortableTimeBuffer[13];
        char calculatedTimeBuffer[13];
        char viewableTimeBuffer[20];
        virtual bool populateTimeDateStrings ();
};

#endif