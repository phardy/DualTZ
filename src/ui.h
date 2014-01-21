#include <pebble.h>

void set_tzname_text(char *TZNameText);
void set_tzoffset_text(char *TZOffsetText);
void set_digital_time(struct tm *time);
void set_date_text(char *DateText);
void update_minute_hand();
void update_hour_hand();
void hour_display_layer_update_callback (Layer *me, GContext* ctx);
void minute_display_layer_update_callback (Layer *me, GContext* ctx);
void battery_state_handler(BatteryChargeState state);
void bluetooth_connection_handler (bool connected);
void lowbattery_handler (bool islow);
void display_init();
void display_deinit();

