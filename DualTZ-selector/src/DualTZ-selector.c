#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "string.h"

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

// Number of TZ regions
#define NUM_REGIONS 10

// Longest file is 143, most are much shorter.
#define MAX_TZ 150
#define TZ_NAME_LEN 15
#define TZ_OFFSET_LEN 5

// tz_index contains offsets for the start of
// each line in the current resource (tz file)
uint16_t tz_index[MAX_TZ];
uint16_t tz_count;

Window root_window;
// Window 
MenuLayer root_menu;
TextLayer textlayer;

typedef char * string;
static string regions[NUM_REGIONS];
uint16_t root_menu_get_num_rows_callback(MenuLayer *me,
					 uint16_t section_index, void *data) {
  return NUM_REGIONS;
}

void root_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer,
				 MenuIndex *cell_index, void *data) {
  menu_cell_basic_draw(ctx, cell_layer, regions[cell_index->row], NULL, NULL);
}

void root_window_load(Window *me) {
  GRect bounds = me->layer.bounds;

  menu_layer_init(&root_menu, bounds);

  MenuLayerCallbacks root_callbacks = {
    .get_num_rows = root_menu_get_num_rows_callback,
    .draw_row = root_menu_draw_row_callback,
    // .select_click = root_menu_select_callback
  };

  menu_layer_set_callbacks(&root_menu, NULL, root_callbacks);
  menu_layer_set_click_config_onto_window(&root_menu, me);
  layer_add_child(&me->layer, menu_layer_get_layer(&root_menu));
}

// Text to be written
// static uint8_t tz_name[TZ_NAME_LEN+1];

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

  window_init(&root_window, "Window Name");
  window_stack_push(&root_window, true /* Animated */);

  resource_init_current_app(&APP_RESOURCES);

  WindowHandlers handlers = {
    .load = root_window_load,
  };
  window_set_window_handlers(&root_window, handlers);

  // Populate the regions array
  regions[0] = "Africa";
  regions[1] = "America";
  regions[2] = "Antarctica";
  regions[3] = "Arctic";
  regions[4] = "Asia";
  regions[5] = "Australia";
  regions[6] = "Atlantic";
  regions[7] = "Europe";
  regions[8] = "Indian";
  regions[9] = "Pacific";

  /* // Read the first element from our file in to a string... */
  /* ResHandle fh = resource_get_handle(RESOURCE_ID_ARCTIC_TZ); */
  /* resource_load_byte_range(fh, 0, tz_name, 16); */
  /* for (int x = 0; x < 16; x++) { */
  /*   if (tz_name[x] == '_') */
  /*     tz_name[x] = ' '; */
  /*   else if (tz_name[x] == ' ') { */
  /*     tz_name[x] = '\0'; */
  /*     break; */
  /*   } */
  /* } */
  /* tz_name[15] = '\0'; */
  /* text_layer_set_text(&textlayer, (char*)tz_name); */
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
