#include "R4RtClock.h"

#ifdef ARDUINO_UNOR4_WIFI

R4RtClock::R4RtClock () {
  RTC.begin();

  syncWithExternalRtc();
}

bool R4RtClock::syncWithExternalRtc () {
  /*bool century = false;
  int month = ds3231.getMonth(century);
  int date = ds3231.getDate();
  int year = ds3231.getYear();
  bool h12Flag;
  bool pmFlag;

  int hour = ds3231.getHour(h12Flag, pmFlag);
  int minute = ds3231.getMinute();
  int second = ds3231.getSecond();*/

  time_t now = RTClib::now().unixtime();

  RTCTime startTime(now);
  RTC.setTime(startTime);

  return RTC.isRunning();
}

bool R4RtClock::setNewDateTime (const char* yymmddHHMMSS) {
  int year = extractInt(yymmddHHMMSS, 2);
  int month = extractInt(yymmddHHMMSS + 2, 2);
  int day = extractInt(yymmddHHMMSS + 4, 2);
  int hour = extractInt(yymmddHHMMSS + 6, 2);
  int minute = extractInt(yymmddHHMMSS + 8, 2);
  int second = extractInt(yymmddHHMMSS + 10, 2);

  DS3231 myRTC;
  myRTC.setClockMode(false);  // set to 24h
  myRTC.setYear(year);
  myRTC.setMonth(month);
  myRTC.setDate(day);
  myRTC.setDoW('w'); // do we really need this
  myRTC.setHour(hour);
  myRTC.setMinute(minute);
  myRTC.setSecond(second);

  return syncWithExternalRtc();
}

bool R4RtClock::isFunctioning () {
  //int year = RTC.getYear();
  //return year > 10 && year < 99;// must be > 2010  and < 2099
  return RTC.isRunning();
}

bool R4RtClock::readLatestTime () {
  RTC.getTime(rtct);  
}

int R4RtClock::getYear() {
  return rtct.getYear();
}

int R4RtClock::getMonth() {
  return ((uint8_t)rtct.getMonth()) + 1;
}

int R4RtClock::getDate() {
  return rtct.getDayOfMonth();
}

int R4RtClock::getSecond() {
  return rtct.getSeconds();
}

int R4RtClock::getMinute() {
  return rtct.getMinutes();
}

int R4RtClock::getHour() {
  return rtct.getHour();
}

#endif