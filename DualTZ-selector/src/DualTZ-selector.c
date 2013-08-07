#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "../../common/config.h"

#ifdef ANDROID
PBL_APP_INFO(SELECTOR_APP_UUID,
	     "Dual-TZ Config", "Kids, Inc.",
	     1, 0, /* App version */
	     DEFAULT_MENU_ICON,
	     APP_INFO_STANDARD_APP);
#else
PBL_APP_INFO(HTTP_UUID,
	     "Dual-TZ Config", "KIDS, Inc.",
	     1, 0, /* App version */
	     DEFAULT_MENU_ICON,
	     APP_INFO_STANDARD_APP);
#endif

#define BUF_SIZE 1024

// Longest file is 143, most are much shorter.
#define MAX_TZ 150
#define TZ_NAME_LEN 15
#define TZ_OFFSET_LEN 5
// tz_index contains offsets for the start of
// each line in the current resource (tz file)
uint16_t tz_index[MAX_TZ];
uint16_t tz_count;

Window window;
MenuLayer root_menu;
TextLayer textlayer;

// Text to be written
static uint8_t tz_name[TZ_NAME_LEN+1];

uint8_t filebuf[BUF_SIZE];
void read_file(void) {
  ResHandle fh = resource_get_handle(RESOURCE_ID_ARCTIC_TZ);
  uint16_t bytesread = BUF_SIZE+1;
  uint16_t index = 0;
  tz_count=0;
  tz_index[tz_count] = index;
  tz_count++;
  while (bytesread >= BUF_SIZE) {
    bytesread = resource_load_byte_range(fh, index, filebuf, BUF_SIZE);
    for (int i=0; i < bytesread; i++) {
      if (filebuf[i] == '\n') {
	tz_index[tz_count] = index;
	tz_count++;
      }
    }
  }
}

void handle_init(AppContextRef ctx) {

  window_init(&window, "Window Name");
  window_stack_push(&window, true /* Animated */);

  resource_init_current_app(&APP_RESOURCES);

  text_layer_init(&textlayer, window.layer.frame);
  text_layer_set_font(&textlayer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_color(&textlayer, GColorBlack);
  text_layer_set_text(&textlayer, "Hello world");
  layer_add_child(&window.layer, &textlayer.layer);
  read_file();

  // Read the first element from our file in to a string...
  ResHandle fh = resource_get_handle(RESOURCE_ID_ARCTIC_TZ);
  resource_load_byte_range(fh, 0, tz_name, 16);
  for (int x = 0; x < 16; x++) {
    if (tz_name[x] == '_')
      tz_name[x] = ' ';
    else if (tz_name[x] == ' ') {
      tz_name[x] = '\0';
      break;
    }
  }
  tz_name[15] = '\0';
  text_layer_set_text(&textlayer, (char*)tz_name);
}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .messaging_info = {
      .buffer_sizes = {
	.inbound = 124,
	.outbound = 256
      }
    }
  };
  app_event_loop(params, &handlers);
}
