#include <pebble.h>

#include "PDutils.h"
#include "tz.h"
#include "ui.h"

#ifdef DEBUG
#include "debug.h"
#endif

static TZInfo DisplayTZ;

static char DateText[] = "  ";
static char TZOffset[] = "      ";
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
    APP_LOG(APP_LOG_LEVEL_INFO, "No tz_name in storage");
    strncpy(tzinfo->tz_name, "local time", TZ_NAME_LEN);
  }
  if (persist_exists(CONFIG_KEY_REMOTE_TZ_OFFSET)) {
    tzinfo->remote_tz_offset = persist_read_int(CONFIG_KEY_REMOTE_TZ_OFFSET);
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "No remote_tz_offset in storage");
    tzinfo->remote_tz_offset = 0;
  }
  if (persist_exists(CONFIG_KEY_LOCAL_TZ_OFFSET)) {
    tzinfo->local_tz_offset = persist_read_int(CONFIG_KEY_LOCAL_TZ_OFFSET);
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "No local_tz_offset in storage");
    tzinfo->local_tz_offset = 0;
  }
}

void write_config(TZInfo *tzinfo) {
  int tz_name = persist_write_string(CONFIG_KEY_REMOTE_TZ_NAME, tzinfo->tz_name);
  int remote_tz_offset = persist_write_int(CONFIG_KEY_REMOTE_TZ_OFFSET, tzinfo->remote_tz_offset);
  int local_tz_offset = persist_write_int(CONFIG_KEY_LOCAL_TZ_OFFSET, tzinfo->local_tz_offset);
  if (tz_name < 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error writing tz_name");
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "tz_name written successfully");
  }
  if (remote_tz_offset < 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error writing remote_tz_offset");
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "remote_tz_offset written successfully");
  }
  if (local_tz_offset < 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error writing local_tz_offset");
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "local_tz_offset written successfully");
  }
}

void update_digital_time(struct tm *time) {
  time_t t1 = p_mktime(time);
  time_t t = t1 - DisplayTZ.local_tz_offset + DisplayTZ.remote_tz_offset;

  struct tm *adjTime;
  adjTime = gmtime(&t);
  set_digital_time(adjTime);
}

void handle_second_tick(struct tm *now, TimeUnits units_changed) {
  update_digital_time(now);

  if (now->tm_sec % 30 == 0) {
    update_minute_hand();
    if (now->tm_min % 2 == 0) {
      update_hour_hand();
    }
  }
  if (now->tm_hour == 0 && now->tm_min == 0) {
    strftime(DateText, sizeof(DateText),
	     "%e", now);
    set_date_text(DateText);
  }
}

void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Data from phone dropped");
}

void in_received_handler(DictionaryIterator *received, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Data packet received");
  Tuple *remote_tz_name_tuple = dict_find(received, CONFIG_KEY_REMOTE_TZ_NAME);
  Tuple *remote_tz_offset_tuple = dict_find(received, CONFIG_KEY_REMOTE_TZ_OFFSET);
  Tuple *local_tz_offset_tuple = dict_find(received, CONFIG_KEY_LOCAL_TZ_OFFSET);

  if (remote_tz_name_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got tz_name");
    strncpy(DisplayTZ.tz_name, remote_tz_name_tuple->value->cstring,
	    TZ_NAME_LEN);
  }
  if (remote_tz_offset_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got local_tz_offset");
    DisplayTZ.remote_tz_offset = remote_tz_offset_tuple->value->int32;
  }
  if (local_tz_offset_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got remote_tz_offset");
    DisplayTZ.local_tz_offset = local_tz_offset_tuple->value->int32;
  }
  set_tzname_text(DisplayTZ.tz_name);
  format_timezone(DisplayTZ.remote_tz_offset, TZOffset);
  set_tzoffset_text(TZOffset);

  time_t t = time(NULL);
  struct tm *now = localtime(&t);
  update_digital_time(now);
}

void handle_init() {
  read_config(&DisplayTZ);
  display_init();

  set_tzname_text(DisplayTZ.tz_name);
  format_timezone(DisplayTZ.remote_tz_offset, TZOffset);
  set_tzoffset_text(TZOffset);

  time_t t = time(NULL);
  struct tm *now = localtime(&t);
  strftime(DateText, sizeof(DateText), "%e", now);
  set_date_text(DateText);
  update_minute_hand();
  update_hour_hand();
  update_digital_time(now);

  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  const uint32_t inbound_size = 64;
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}

void handle_deinit() {
  write_config(&DisplayTZ);
  display_deinit();
}

int main() {
  handle_init();
  app_event_loop();
  handle_deinit();
}
