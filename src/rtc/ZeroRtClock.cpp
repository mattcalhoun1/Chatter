#include "ZeroRtClock.h"

#ifdef ARDUINO_SAMD_MKRZERO

ZeroRtClock::ZeroRtClock () {
  rtc.begin(); // initialize RTC

  syncWithExternalRtc();
}

bool ZeroRtClock::syncWithExternalRtc () {
  bool century = false;
  bool h12Flag;
  bool pmFlag;

  // Set the time
  rtc.setHours(ds3231.getHour(h12Flag, pmFlag));
  rtc.setMinutes(ds3231.getMinute());
  rtc.setSeconds(ds3231.getSecond());

  // Set the date
  rtc.setDay(ds3231.getDate());
  rtc.setMonth(ds3231.getMonth(century));
  rtc.setYear(ds3231.getYear());
}

int ZeroRtClock::getYear() {
  return rtc.getYear();
}

int ZeroRtClock::getMonth() {
  return rtc.getMonth();
}


int ZeroRtClock::getDate() {
  return rtc.getDay();
}

int ZeroRtClock::getSecond() {
  return rtc.getSeconds();
}

int ZeroRtClock::getMinute() {
  return rtc.getMinutes();
}

int ZeroRtClock::getHour() {
  return rtc.getHours();
}

bool ZeroRtClock::isFunctioning () {
  int year = rtc.getYear();
  return year > 10 && year < 99;// must be > 2010  and < 2099
}

bool ZeroRtClock::readLatestTime () {
  // doesn't need to do anything in this implementation , since we directly use zero time library
}

bool ZeroRtClock::setNewDateTime (const char* yymmddHHMMSS) {
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

#endif