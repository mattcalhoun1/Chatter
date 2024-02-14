#include "DueRtClock.h"

#if defined(ARDUINO_SAM_DUE)

DueRtClock::DueRtClock () {
  rtcd.begin();

  syncWithExternalRtc();
}

bool DueRtClock::syncWithExternalRtc () {
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

  //RTCTime startTime(now);
  rtcd.setClock(now);

  return isFunctioning();
}

bool DueRtClock::setNewDateTime (const char* yymmddHHMMSS) {
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

bool DueRtClock::isFunctioning () {
  int year = getYear();
  return year > 2010 && year < 2099;
}

bool DueRtClock::readLatestTime () {
  //RTC.getTime(rtcd);  
}

int DueRtClock::getYear() {
  return rtcd.getYear();
}

int DueRtClock::getMonth() {
  return ((uint8_t)rtcd.getMonth()) + 1;
}

int DueRtClock::getDate() {
  return rtcd.getDay();
}

int DueRtClock::getSecond() {
  return rtcd.getSeconds();
}

int DueRtClock::getMinute() {
  return rtcd.getMinutes();
}

int DueRtClock::getHour() {
  return rtcd.getHours();
}

#endif