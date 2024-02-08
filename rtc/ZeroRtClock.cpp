#include "ZeroRtClock.h"

#ifdef ARDUINO_SAMD_MKRZERO

ZeroRtClock::ZeroRtClock () {
  DS3231 ds3231; // get initial timeDate

  bool century = false;
  int month = ds3231.getMonth(century);
  int date = ds3231.getDate();
  int year = ds3231.getYear();
  bool h12Flag;
  bool pmFlag;

  int hour = ds3231.getHour(h12Flag, pmFlag);
  int minute = ds3231.getMinute();
  int second = ds3231.getSecond();


  rtc.begin(); // initialize RTC

  // Set the time
  rtc.setHours(hour);
  rtc.setMinutes(minute);
  rtc.setSeconds(second);

  // Set the date
  rtc.setDay(date);
  rtc.setMonth(month);
  rtc.setYear(year);
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

#endif