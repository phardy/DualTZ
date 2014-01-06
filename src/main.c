#include <pebble.h>
#include "config.h"
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
  CONFIG_KEY_LOCAL_TZ_OFFSET = 0x5F,
  CONFIG_KEY_BTDISCO_NOTIFICATION = 0x60,
  CONFIG_KEY_LOWBAT_NOTIFICATION = 0x61
};

void read_config(TZInfo *tzinfo) {
  if (persist_exists(CONFIG_KEY_REMOTE_TZ_NAME)) {
    persist_read_string(CONFIG_KEY_REMOTE_TZ_NAME,
			tzinfo->tz_name, TZ_NAME_LEN);
    tzinfo->tz_name[TZ_NAME_LEN] = '\0';
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "No tz_name in storage");
    strncpy(tzinfo->tz_name, "not configured", TZ_NAME_LEN);
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
  if (persist_exists(CONFIG_KEY_BTDISCO_NOTIFICATION)) {
    set_btdisco_notification(persist_read_bool(CONFIG_KEY_BTDISCO_NOTIFICATION));
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "No btdisco_notification in storage");
  }
  if (persist_exists(CONFIG_KEY_LOWBAT_NOTIFICATION)) {
    set_lowbat_notification(persist_read_bool(CONFIG_KEY_LOWBAT_NOTIFICATION));
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "No lowbat_notification in storage");
  }
}

void write_config(TZInfo *tzinfo) {
  int tz_name = persist_write_string(CONFIG_KEY_REMOTE_TZ_NAME, tzinfo->tz_name);
  int remote_tz_offset = persist_write_int(CONFIG_KEY_REMOTE_TZ_OFFSET, tzinfo->remote_tz_offset);
  int local_tz_offset = persist_write_int(CONFIG_KEY_LOCAL_TZ_OFFSET, tzinfo->local_tz_offset);
  int btdisco_notification = persist_write_bool(CONFIG_KEY_BTDISCO_NOTIFICATION, get_btdisco_notification());
  int lowbat_notification = persist_write_bool(CONFIG_KEY_LOWBAT_NOTIFICATION, get_lowbat_notification());
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
  if (btdisco_notification < 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error writing btdisco_notification");
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "btdisco_notification written successfully");
  }
  if (lowbat_notification < 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error writing lowbat_notification");
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "lowbat_notification written successfully");
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
  Tuple *btdisco_notification_tuple = dict_find(received, CONFIG_KEY_BTDISCO_NOTIFICATION);
  Tuple *lowbat_notification_tuple = dict_find(received, CONFIG_KEY_LOWBAT_NOTIFICATION);

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
  if (btdisco_notification_tuple) {
    // We're able to send these as bools, but can't receive them like that.
    // Testing shows that a boolean true is cast to an int of 116 by
    // AppMessage, and boolean false is cast to 102.
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got btdisco_notification");
    if (btdisco_notification_tuple->value->int8 == 116) {
      set_btdisco_notification(true);
    } else {
      set_btdisco_notification(false);
    }
  } else {
    set_btdisco_notification(false);
  }
  if (lowbat_notification_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got lowbat_notification");
    if (lowbat_notification_tuple->value->int8 == 116) {
      set_lowbat_notification(true);
    } else {
      set_lowbat_notification(false);
    }
  } else {
    set_lowbat_notification(false);
  }
  set_tzname_text(DisplayTZ.tz_name);
  format_timezone(DisplayTZ.remote_tz_offset, TZOffset);
  set_tzoffset_text(TZOffset);

  time_t t = time(NULL);
  struct tm *now = localtime(&t);
  update_digital_time(now);

  write_config(&DisplayTZ);
}

void handle_init() {
  config_init();
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
  display_deinit();
}

int main() {
  handle_init();
  app_event_loop();
  handle_deinit();
}
