#include <pebble.h>

#include "PDutils.h"
#include "tz.h"
#include "ui.h"

#ifdef DEBUG
#include "debug.h"
#endif

static TZInfo DisplayTZ;

static char DateText[] = "  ";
static char DigitalTZOffset[] = "      ";

// data received from the config page
enum {
  CONFIG_KEY_REMOTE_TZ_NAME = 0x5D,
  CONFIG_KEY_REMOTE_TZ_OFFSET = 0x5E,
  CONFIG_KEY_LOCAL_TZ_OFFSET = 0x5F
};

void read_config(TZInfo *tzinfo) {
  if (persist_exists(CONFIG_KEY_REMOTE_TZ_NAME)) {
    persist_read_string(CONFIG_KEY_REMOTE_TZ_NAME,
			tzinfo->tz_name, TZ_NAME_LEN);
    tzinfo->tz_name[TZ_NAME_LEN] = '\0';
  } else {
    strncpy(tzinfo->tz_name, "local time", TZ_NAME_LEN);
  }
  if (persist_exists(CONFIG_KEY_REMOTE_TZ_OFFSET)) {
    tzinfo->remote_tz_offset = persist_read_int(CONFIG_KEY_REMOTE_TZ_OFFSET);
  } else {
    tzinfo->remote_tz_offset = 0;
  }
  if (persist_exists(CONFIG_KEY_LOCAL_TZ_OFFSET)) {
    tzinfo->local_tz_offset = persist_read_int(CONFIG_KEY_LOCAL_TZ_OFFSET);
  } else {
    tzinfo->local_tz_offset = 0;
  }
}

void update_digital_time(struct tm *time) {
  time_t t1 = p_mktime(time);
  time_t t = t1 - DisplayTZ.local_tz_offset + DisplayTZ.remote_tz_offset;

  struct tm *adjTime;
  adjTime = gmtime(&t);
  set_digital_text(adjTime);
}

void handle_second_tick(struct tm *now, TimeUnits units_changed) {
  set_digitals_text(now);

  if (now->tm_sec % 30 == 0) {
    update_minute_hand();
    if (now->tm_min % 2 == 0) {
      update_hour_hand();
    }
  }
  if (now->tm_sec == 0) {
    update_digital_time(now);
  }
  if (now->tm_hour == 0 && now->tm_min == 0) {
    strftime(DateText, sizeof(DateText),
	     "%e", now);
    set_date_text(DateText);
  }
}

void handle_init() {
  read_config(&DisplayTZ);
  display_init();

  time_t t = time(NULL);
  struct tm *now = localtime(&t);
  strftime(DateText, sizeof(DateText), "%e", now);
  set_date_text(DateText);
  update_minute_hand();
  update_hour_hand();
  update_digital_time(now);

  /* app_message_register_inbox_received(in_received_handler); */
  /* app_message_register_inbox_dropped(in_dropped_handler); */
  /* const uint32_t inbound_size = 64; */
  /* const uint32_t outbound_size = 64; */
  /* app_message_open(inbound_size, outbound_size); */

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}

void handle_deinit() {
  display_deinit();
}

int main() {
  handle_init();
  app_event_loop();
  handle_deinit();
}
