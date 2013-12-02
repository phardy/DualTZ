#include <pebble.h>
#include <ctype.h>

#include "PDutils.h"

#include "tz.h"

void format_timezone(int32_t offset, char *str) {
  offset = offset/60;
  int tz_hours = offset / 60;
  int tz_minutes = offset % 60;
  snprintf(str, TZ_OFFSET_LEN, "%+02d%02d", tz_hours, tz_minutes);
  // if (tz_minutes == 0) {
  //   snprintf(str, TZ_OFFSET_LEN, "%+02d", tz_hours);
  // } else {
  //   snprintf(str, TZ_OFFSET_LEN, "%+02d:%02d", tz_hours, tz_minutes);
  // }
}
