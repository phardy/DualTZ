#include "stdlib.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#include "xprintf.h"

#include "common.h"

void format_timezone(TZInfo *tz, char *str) {
  // xprintf doesn't seem to support the + formatting flag
  if (tz->tz_hours < 0) {
    if (tz->tz_minutes == 0) {
      xsprintf(str, "%02d", tz->tz_hours);
    } else {
      xsprintf(str, "%02d:%02d", tz->tz_hours, tz->tz_minutes);
    }
  } else {
    if (tz->tz_minutes == 0) {
      xsprintf(str, "+%02d", tz->tz_hours);
    } else {
      xsprintf(str, "+%02d:%02d", tz->tz_hours, tz->tz_minutes);
    }
  }
}
