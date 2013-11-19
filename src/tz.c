#include <pebble.h>
#include <ctype.h>

#include "PDutils.h"

#include "tz.h"

void format_timezone(TZInfo *tz, char *str) {
  int tz_hours = tz->tz_offset / 60;
  int tz_minutes = tz->tz_offset % 60;
  // xprintf doesn't seem to support the + formatting flag
  if (tz_hours < 0) {
    if (tz_minutes == 0) {
      snprintf(str, TZ_OFFSET_LEN, "%02d", tz_hours);
    } else {
      snprintf(str, TZ_OFFSET_LEN, "%02d:%02d", tz_hours, tz_minutes);
    }
  } else {
    if (tz_minutes == 0) {
      snprintf(str, TZ_OFFSET_LEN, "+%02d", tz_hours);
    } else {
      snprintf(str, TZ_OFFSET_LEN, "+%02d:%02d", tz_hours, tz_minutes);
    }
  }
}
