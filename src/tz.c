#include <pebble.h>
#include <ctype.h>

#include "PDutils.h"

#include "tz.h"

void format_timezone(TZInfo *tz, char *str) {
  // xprintf doesn't seem to support the + formatting flag
  if (tz->tz_hours < 0) {
    if (tz->tz_minutes == 0) {
      sprintf(str, "%02d", tz->tz_hours);
    } else {
      sprintf(str, "%02d:%02d", tz->tz_hours, tz->tz_minutes);
    }
  } else {
    if (tz->tz_minutes == 0) {
      sprintf(str, "+%02d", tz->tz_hours);
    } else {
      sprintf(str, "+%02d:%02d", tz->tz_hours, tz->tz_minutes);
    }
  }
}

// Parses a TZ string "TZ name\tTZ offset"
// in to the given TZInfo struct.
void parse_timezone(char *str, TZInfo *tz) {
  const char sep[] = "\t:";
  const char *token;
  // Extract TZ name
  token = p_strtok(str, sep);
  if (token != NULL) {
    strncpy(tz->tz_name, token, TZ_NAME_LEN);
    tz->tz_name[TZ_NAME_LEN] = '\0'; // In case we have a long name
  }
  // Extract TZ offset
  token = p_strtok(NULL, sep);
  if (token != NULL) {
    long tz_hours = p_strtol(token, (char **)NULL, 10);
    tz->tz_hours = (int8_t)tz_hours;
  } else {
    tz->tz_hours = 0;
  }
  token = p_strtok(NULL, sep);
  if (token != NULL) {
    long tz_minutes = p_strtol(token, (char **)NULL, 10);
    tz->tz_minutes = (int8_t)tz_minutes;
  } else {
    tz->tz_minutes = 0;
  }

  token = p_strtok(NULL, sep);
  if (token != NULL && token[0] == '1') {
    tz->tz_dst = true;
  } else {
    tz->tz_dst = false;
  }
}
