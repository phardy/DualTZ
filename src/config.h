#include <pebble.h>

typedef struct {
  bool lowbat;
  bool btdisco;
} Config;

void config_init();
bool get_lowbat_notification();
bool get_btdisco_notification();
void set_lowbat_notification(bool state);
void set_btdisco_notification(bool state);
