#include <pebble.h>

#include "PDutils.h"
#include "tz.h"
#include "ui.h"

#ifdef DEBUG
#include "debug.h"
#endif

static TZInfo DisplayTZ;
int32_t localTZOffset;

static char DigitalTimeText[] = "  :  ";
static char DigitalTimeSText[] = "  ";
static char DateText[] = "  ";
static char DigitalTZOffset[] = "      ";
static char *DigitalTimeFormat;

// data received from the config page
enum {
  CONFIG_KEY_REMOTE_TZ_NAME = 0x5D,
  CONFIG_KEY_REMOTE_REMOTE_TZ_OFFSET = 0x5E,
  CONFIG_KEY_LOCAL_REMOTE_TZ_OFFSET = 0x5F
};

void update_digital_time(struct tm *time) {
  time_t t1 = p_mktime(time);
  int32_t t = (int32_t)t1 + localTZOffset;

  struct tm *adjTime;
  adjTime = gmtime(&t);
  strftime(DigitalTimeText, sizeof(DigitalTimeText),
	   DigitalTimeFormat, adjTime);
  set_digital_text(DigitalTimeText);
}

void in_dropped_handler(AppMessageResult reason, void *context) {
  // stub. request again?
}

void apply_stored_config() {
  if (persist_exists(CONFIG_KEY_REMOTE_TZ_NAME)) {
    persist_read_string(CONFIG_KEY_REMOTE_TZ_NAME,
			DisplayTZ.tz_name, TZ_NAME_LEN);
    DisplayTZ.tz_name[TZ_NAME_LEN] = '\0';
  } else {
    strncpy(DisplayTZ.tz_name, "local time", TZ_NAME_LEN);
  }
  set_tzname_text(DisplayTZ.tz_name);
  if (persist_exists(CONFIG_KEY_REMOTE_REMOTE_TZ_OFFSET)) {
    DisplayTZ.remote_tz_offset = persist_read_int(CONFIG_KEY_REMOTE_REMOTE_TZ_OFFSET);
    format_timezone(&DisplayTZ, DigitalTZOffset);
    set_tzoffset_text(DigitalTZOffset);
  } else {
    // Don't write a timezone to the display here.
    DisplayTZ.remote_tz_offset = 0;
  }
  if (persist_exists(CONFIG_KEY_LOCAL_REMOTE_TZ_OFFSET)) {
    localTZOffset = DisplayTZ.remote_tz_offset - 
      persist_read_int(CONFIG_KEY_LOCAL_REMOTE_TZ_OFFSET);
  } else {
    localTZOffset = 0;
  }

  time_t t = time(NULL);
  struct tm *now;
  now = localtime(&t);
  update_digital_time(now);
}

void in_received_handler(DictionaryIterator *received, void *context) {
  Tuple *remote_tz_name_tuple = dict_find(received, CONFIG_KEY_REMOTE_TZ_NAME);
  Tuple *remote_remote_tz_offset_tuple = dict_find(received, CONFIG_KEY_REMOTE_REMOTE_TZ_OFFSET);
  Tuple *local_remote_tz_offset_tuple = dict_find(received, CONFIG_KEY_LOCAL_REMOTE_TZ_OFFSET);

  // Right now we only ever get all three in one packet
  if (remote_tz_name_tuple && remote_remote_tz_offset_tuple && local_remote_tz_offset_tuple) {
    int remote_tz_name_write = persist_write_string(CONFIG_KEY_REMOTE_TZ_NAME,
						    remote_tz_name_tuple->value->cstring);
    int remote_remote_tz_offset_write = persist_write_int(CONFIG_KEY_REMOTE_REMOTE_TZ_OFFSET,
						   remote_remote_tz_offset_tuple->value->int32);
    int local_remote_tz_offset_write = persist_write_int(CONFIG_KEY_LOCAL_REMOTE_TZ_OFFSET,
						  local_remote_tz_offset_tuple->value->int32);
#ifdef DEBUG
    debug_storage_write(remote_tz_name_write);
    debug_storage_write(remote_remote_tz_offset_write);
    debug_storage_write(local_remote_tz_offset_write);
#endif

    apply_stored_config();
  }
}


void handle_second_tick(struct tm *now, TimeUnits units_changed) {
  strftime(DigitalTimeSText, sizeof(DigitalTimeSText),
	   "%S", now);
  set_digitals_text(DigitalTimeSText);

  if (now->tm_sec % 30 == 0) {
    update_minute_hand();
    if (now->tm_min % 2 == 0) {
      update_hour_hand();
    }

    update_digital_time(now);
  }
  if (now->tm_hour == 0 && now->tm_min == 0) {
    strftime(DateText, sizeof(DateText),
	     "%e", now);
    set_date_text(DateText);
  }
}

void handle_init() {

  display_init();

  strcpy(DisplayTZ.tz_name, "UTC");
  DisplayTZ.remote_tz_offset = 0;

  if (clock_is_24h_style()) {
    DigitalTimeFormat = "%H:%M";
  } else {
    DigitalTimeFormat = "%l:%M";
  }

  // write current time to display
  apply_stored_config();
  time_t t = time(NULL);
  struct tm *now = localtime(&t);
  strftime(DateText, sizeof(DateText), "%e", now);
  set_date_text(DateText);
  update_minute_hand();
  update_hour_hand();

  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  const uint32_t inbound_size = 64;
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);

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
