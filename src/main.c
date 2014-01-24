#include <pebble.h>
#include "config.h"
#include "PDutils.h"
#include "tz.h"
#include "ui.h"

#define LOWBAT_LEVEL 20

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
  if (persist_exists(CONFIG_KEY_REMOTE_TZ_NAME)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Deleting tz_name");
    persist_delete(CONFIG_KEY_REMOTE_TZ_NAME);
  }
  int tz_name = persist_write_string(CONFIG_KEY_REMOTE_TZ_NAME, tzinfo->tz_name);
  if (tz_name < 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error writing tz_name");
    set_tzname_text("save error");
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "tz_name written successfully");
  }

  if (persist_exists(CONFIG_KEY_REMOTE_TZ_OFFSET)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Deleting remote_tz_offset");
    persist_delete(CONFIG_KEY_REMOTE_TZ_OFFSET);
  }
  int remote_tz_offset = persist_write_int(CONFIG_KEY_REMOTE_TZ_OFFSET, tzinfo->remote_tz_offset);
  if (remote_tz_offset < 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error writing remote_tz_offset");
    set_tzname_text("save error");
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "remote_tz_offset written successfully");
  }

  if (persist_exists(CONFIG_KEY_LOCAL_TZ_OFFSET)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Deleting local_tz_offset");
    persist_delete(CONFIG_KEY_LOCAL_TZ_OFFSET);
  }
  int local_tz_offset = persist_write_int(CONFIG_KEY_LOCAL_TZ_OFFSET, tzinfo->local_tz_offset);
  if (local_tz_offset < 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error writing local_tz_offset");
    set_tzname_text("save error");
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "local_tz_offset written successfully");
  }

  if (persist_exists(CONFIG_KEY_BTDISCO_NOTIFICATION)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Deleting btdisco_notification");
    persist_delete(CONFIG_KEY_BTDISCO_NOTIFICATION);
  }
  int btdisco_notification = persist_write_bool(CONFIG_KEY_BTDISCO_NOTIFICATION, get_btdisco_notification());
  if (btdisco_notification < 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error writing btdisco_notification");
    set_tzname_text("save error");
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "btdisco_notification written successfully");
  }

  if (persist_exists(CONFIG_KEY_LOWBAT_NOTIFICATION)) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Deleting lowbat_notification");
    persist_delete(CONFIG_KEY_LOWBAT_NOTIFICATION);
  }
  int lowbat_notification = persist_write_bool(CONFIG_KEY_LOWBAT_NOTIFICATION, get_lowbat_notification());
  if (lowbat_notification < 0) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Error writing lowbat_notification");
    set_tzname_text("save error");
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
  (void)units_changed;
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
  if (now->tm_sec % 60 == 0) {
    if (get_lowbat_notification()) {
      BatteryChargeState state = battery_state_service_peek();
      if ((state.charge_percent <= LOWBAT_LEVEL) && !state.is_charging) {
	lowbattery_handler(true);
      } else {
	lowbattery_handler(false);
      }
    }
  }
}

void in_dropped_handler(AppMessageResult reason, void *context) {
  (void)reason;
  (void)context;
  APP_LOG(APP_LOG_LEVEL_ERROR, "Data from phone dropped");
}

void in_received_handler(DictionaryIterator *received, void *context) {
  (void)context;
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
    // AppMessage seems to replace ' ' with '+'. Fix that now.
    for (int i=0; i<TZ_NAME_LEN; i++) {
      if (DisplayTZ.tz_name[i] == '+') {
	DisplayTZ.tz_name[i] = ' ';
      }
    }
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
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got btdisco_notification");
    set_btdisco_notification((bool)btdisco_notification_tuple->value->int8);
  } else {
    set_btdisco_notification(false);
  }
  if (lowbat_notification_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Got lowbat_notification");
    set_lowbat_notification((bool)lowbat_notification_tuple->value->int8);
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
