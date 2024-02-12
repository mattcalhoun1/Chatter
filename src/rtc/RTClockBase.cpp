#include "RTClockBase.h"

bool RTClockBase::populateTimeDateStrings() {
  readLatestTime();
  int month = getMonth();
  int date = getDate();
  int year = getYear();

  int hour = getHour();
  int minute = getMinute();
  int second = getSecond();

  sprintf(viewableTimeBuffer, viewableTimeFormat, month, date, year, hour, minute, second);        
  sprintf(sortableTimeBuffer, sortableTimeFormat, year > 2000 ? year - 2000 : year, month, date, hour, minute, second);        

  return isFunctioning();
}

const char* RTClockBase::getSortableTime () {
    populateTimeDateStrings();
    return sortableTimeBuffer;
}

const char* RTClockBase::getViewableTime () {
    populateTimeDateStrings();
    return viewableTimeBuffer;
}

int RTClockBase::extractInt (const char* digits, int numDigits) {
  int val;
  for (int i = 0; i < numDigits; i++) {
    val += (digits[i] - 48) * pow(10, (numDigits-1) - i);
  }
  return val;
}

const char* RTClockBase::getSortableTimePlusSeconds (int seconds) {
  readLatestTime();
  int month = getMonth();
  int date = getDate();
  int year = getYear();

  int hour = getHour();
  int minute = getMinute();
  int second = getSecond();


  tmElements_t T1;
  tmElements_t T2;

  T1.Hour = getHour();
  T1.Minute = getMinute(); 
  T1.Second = getSecond(); 
  T1.Day = getDate();
  T1.Month = getMonth(); 
  int shiftedYear = getYear();
  if (shiftedYear < 2000) {
    shiftedYear += 2000;
  }
  T1.Year = shiftedYear - 1970;

  time_t calcTime = makeTime( T1 );
  calcTime += seconds;

  breakTime(calcTime, T2);

  sprintf(calculatedTimeBuffer, sortableTimeFormat, T2.Year - 30, T2.Month, T2.Day, T2.Hour, T2.Minute, T2.Second);
  return calculatedTimeBuffer;
}