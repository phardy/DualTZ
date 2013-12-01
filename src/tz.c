#include <pebble.h>
#include <ctype.h>

#include "PDutils.h"

#include "tz.h"

void format_timezone(int32_t offset, char *str) {
  int tz_hours = offset / 3600;
  // TODO: test this - not sure if 3600 or 60.
  int tz_minutes = offset % 3600;
  // TODO: Update these. Probably not requried any more.
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
