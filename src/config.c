#include "config.h"
#include "ui.h"

static Config watch_config;

void config_init() {
  watch_config.lowbat = false;
  watch_config.btdisco = false;
}

bool get_lowbat_notification() {
  return watch_config.lowbat;
}

bool get_btdisco_notification() {
  return watch_config.btdisco;
}

void set_lowbat_notification(bool state) {
  watch_config.lowbat = state;
}

void set_btdisco_notification(bool state) {
  watch_config.btdisco = state;
  if (state) {
    bluetooth_connection_service_subscribe(bluetooth_connection_handler);
  } else {
    bluetooth_connection_service_unsubscribe();
  }
}
