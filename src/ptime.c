#include "pebble_os.h"
#include "pebble_app.h"
#include "ptime.h"

#include "time.h"

#define SECSPERMIN 60L
#define MINSPERHOUR 60L
#define HOURSPERDAY 24L
#define SECSPERHOUR (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY (SECSPERHOUR * HOURSPERDAY)
#define DAYSPERWEEK 7
#define MONSPERYEAR 12

#define _SEC_IN_MINUTE 60L
#define _SEC_IN_HOUR 3600L
#define _SEC_IN_DAY 86400L

static _CONST int DAYS_IN_MONTH[12] =
  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

#define _DAYS_IN_MONTH(x) ((x == 1) ? days_in_feb : DAYS_IN_MONTH[x])

static _CONST int _DAYS_BEFORE_MONTH[12] =
  {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

#define _ISLEAP(y) (((y) % 4) == 0 && (((y) % 100) != 0 || (((y)+1900) % 400) == 0))
#define _DAYS_IN_YEAR(year) (_ISLEAP(year) ? 366 : 365)

time_t pmktime (PblTm *tm) {
  time_t tim = 0;
  long days = 0;
  int year;

  /* compute hours, minutes, seconds */
  tim += tm->tm_sec + (tm->tm_min * _SEC_IN_MINUTE) +
    (tm->tm_hour * _SEC_IN_HOUR);

  /* days in this year */
  days += tm->tm_yday;

  /* compute day of the year */
  tm->tm_yday = days;

    if (tm->tm_year > 10000
	|| tm->tm_year < -10000) {
      return (time_t) -1;
    }

  /* compute days in other years */
  if (tm->tm_year > 70) {
    for (year = 70; year < tm->tm_year; year++)
      days += _DAYS_IN_YEAR (year);
  } else if (tm->tm_year < 70) {
    for (year = 69; year > tm->tm_year; year--)
      days -= _DAYS_IN_YEAR (year);
    days -= _DAYS_IN_YEAR (year);
  }

  /* compute total seconds */
  tim += (days * _SEC_IN_DAY);

  return tim;
}

static _CONST int mon_lengths[2][MONSPERYEAR] = {
  {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
  {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

static _CONST int year_lengths[2] = {
  365,
  366
};

PblTm plocaltime (const time_t *timep) {
  PblTm res;
  long days, rem;

  days = ((int32_t)*timep) / SECSPERDAY;
  rem = ((int32_t)*timep) % SECSPERDAY;
  while (rem < 0) {
    rem += SECSPERDAY;
    --days;
  }
  while (rem >= SECSPERDAY) {
    rem += SECSPERDAY;
    ++days;
  }

  /* compute hour, min, and sec */
  res.tm_hour = (int) (rem / SECSPERHOUR);
  rem %= SECSPERHOUR;
  res.tm_min = (int) (rem / SECSPERMIN);
  res.tm_sec = (int) (rem % SECSPERMIN);

  return res;
}
