#include "pebble_os.h"

#include "stdlib.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"

#include "xprintf.h"
#include "PDutils.h"

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

// Parses a TZ string "TZ name\tTZ offset"
// in to the given TZInfo struct.
void parse_timezone(char *str, TZInfo *tz) {
  const char sep[] = "\t:";
  char *token;
  // Extract TZ name
  token = pstrtok(str, sep);
  if (token != NULL) {
    strncpy(tz->tz_name, token, TZ_NAME_LEN);
    tz->tz_name[TZ_NAME_LEN] = '\0'; // In case we have a long name
  }
  // Extract TZ offset
  token = pstrtok(NULL, sep);
  if (token != NULL) {
    long tz_hours;
    xatoi(&token, &tz_hours);
    tz->tz_hours = (int8_t)tz_hours;
  } else {
    tz->tz_hours = 0;
  }
  token = pstrtok(NULL, sep);
  if (token != NULL) {
    long tz_minutes;
    xatoi(&token, &tz_minutes);
    tz->tz_minutes = (int8_t)tz_minutes;
  } else {
    tz->tz_minutes = 0;
  }

  token = pstrtok(NULL, sep);
  if (token != NULL && token[0] == '1') {
    tz->tz_dst = true;
  } else {
    tz->tz_dst = false;
  }
}
