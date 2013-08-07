#include "pebble_os.h"
#include "pebble_app.h"
#include "ptime.h"

#include "time.h"

#define SECS_PER_MIN 60L
#define MINS_PER_HOUR 60L
#define HOURS_PER_DAY 24L
#define SECS_PER_HOUR (SECS_PER_MIN * MINS_PER_HOUR)
#define SECS_PER_DAY (SECS_PER_HOUR * HOURS_PER_DAY)
#define DAYS_PER_WEEK 7
#define MONS_PER_YEAR 12

#define _ISLEAP(y) (((y) % 4) == 0 && (((y) % 100) != 0 || (((y)+1900) % 400) == 0))
#define _DAYS_IN_YEAR(year) (_ISLEAP(year) ? 366 : 365)

time_t pmktime (PblTm *tm) {
  time_t tim = 0;
  long days = 0;
  int year;

  /* compute hours, minutes, seconds */
  tim += tm->tm_sec + (tm->tm_min * SECS_PER_MIN) +
    (tm->tm_hour * SECS_PER_HOUR);

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
  tim += (days * SECS_PER_DAY);

  return tim;
}

static _CONST int mon_lengths[2][MONS_PER_YEAR] = {
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

  days = ((int32_t)*timep) / SECS_PER_DAY;
  rem = ((int32_t)*timep) % SECS_PER_DAY;
  while (rem < 0) {
    rem += SECS_PER_DAY;
    --days;
  }
  while (rem >= SECS_PER_DAY) {
    rem += SECS_PER_DAY;
    ++days;
  }

  /* compute hour, min, and sec */
  res.tm_hour = (int) (rem / SECS_PER_HOUR);
  rem %= SECS_PER_HOUR;
  res.tm_min = (int) (rem / SECS_PER_MIN);
  res.tm_sec = (int) (rem % SECS_PER_MIN);

  return res;
}
