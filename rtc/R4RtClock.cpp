#include "R4RtClock.h"

#ifdef ARDUINO_UNOR4_WIFI

R4RtClock::R4RtClock () {
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


  //rtc.begin(); // initialize RTC
  RTC.begin();

  time_t now = RTClib::now().unixtime();

  //RTCTime startTime(date, month, year + 2000, hour, minute, second);
  RTCTime startTime(now);
  RTC.setTime(startTime);

  // Set the time
  //rtc.setHours(hour);
  //rtc.setMinutes(minute);
  //rtc.setSeconds(second);

  // Set the date
  //rtc.setDay(date);
  //rtc.setMonth(month);
  //rtc.setYear(year);
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