#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "http.h"
#include "xprintf.h"
#include "PDutils.h"

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

// Longest file is 125, most are much shorter.
#define MAX_TZ 130
#define TZ_NAME_LEN 15
#define TZ_OFFSET_LEN 5

Window root_window;
Window region_window;
Window zone_window;
MenuLayer root_menu;
MenuLayer region_menu;
MenuLayer zone_menu;

TZInfo SelectedTZ;
TZInfo RemoteTZ;

// Number of TZ regions
#define NUM_REGIONS 10
typedef char * string;
static string regions[NUM_REGIONS];
uint32_t region_resources[NUM_REGIONS] = {
  RESOURCE_ID_AFRICA_TZ,
  RESOURCE_ID_AMERICA_TZ,
  RESOURCE_ID_ANTARCTICA_TZ,
  RESOURCE_ID_ARCTIC_TZ,
  RESOURCE_ID_ASIA_TZ,
  RESOURCE_ID_ATLANTIC_TZ,
  RESOURCE_ID_AUSTRALIA_TZ,
  RESOURCE_ID_EUROPE_TZ,
  RESOURCE_ID_INDIAN_TZ,
  RESOURCE_ID_PACIFIC_TZ
};

// tz_index contains offsets for the start of
// each line in the current resource (tz file)
static uint16_t tz_index[MAX_TZ];
static uint16_t tz_count;

ResHandle current_resource_handle;
void read_file(uint32_t resource_id) {
  current_resource_handle = resource_get_handle(resource_id);
  uint16_t bytesread = BUF_SIZE+1;
  uint16_t index = 0;
  uint8_t filebuf[BUF_SIZE];
  tz_count = 0;
  tz_index[tz_count] = index;
  tz_count++;
  while (bytesread >= BUF_SIZE) {
    bytesread = resource_load_byte_range(current_resource_handle, index, filebuf, BUF_SIZE);
    for (int i=0; i < bytesread; i++) {
      if (filebuf[i] == '\n') {
	tz_index[tz_count] = index+i+1;
	tz_count++;
      }
    }
    index += bytesread;
  }
  tz_count--; // Deal with the newline at the end of the file
}

// Fetches the timezone stored at the given index in the
// current resource handle, parses it in to SelectedTZ
void fetch_time_zone(uint16_t idx) {
  // Making assumptions about the maximum line length and format here...
  uint8_t line[80];
  resource_load_byte_range(current_resource_handle, tz_index[idx],
			   line, 80);
  // Find the newline and replace it with a string terminator
  for (int i=0; i<80; i++) {
    if (line[i] == '\0') {
      break;
    } else if (line[i] == '\n') {
      line[i] = '\0';
      break;
    }
  }
  line[79] = '\0';

  const char sep[] = " ";
  char *token;
  // Extract TZ name
  token = pstrtok((char*)line, sep);
  strncpy(SelectedTZ.tz_name, token, TZ_NAME_LEN);
  // formatting cleanup
  for (int i=0; i<TZ_NAME_LEN; i++) {
    if (SelectedTZ.tz_name[i] == '_')
      SelectedTZ.tz_name[i] = ' ';
  }
  SelectedTZ.tz_name[TZ_NAME_LEN] = '\0'; // In case we have a long name
  // Extract TZ offset
  token = pstrtok(NULL, sep);
  strncpy(SelectedTZ.tz_offset, token, TZ_OFFSET_LEN);
  SelectedTZ.tz_offset[TZ_OFFSET_LEN] = '\0';
  // Extract TZ seconds
  token = pstrtok(NULL, (char*)'\n');
  long offset;
  xatoi(&token, &offset);
  SelectedTZ.tz_seconds = (int16_t)offset;
}

uint16_t root_menu_get_num_rows_callback(MenuLayer *me,
					 uint16_t section_index, void *data) {
  return 2;
}

void root_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer,
				 MenuIndex *cell_index, void *data) {
  char sub[TZ_NAME_LEN+TZ_OFFSET_LEN+2]; // +3 so we can add 2-char sep
  switch (cell_index->row) {
  case 0 :
    strcpy(sub, RemoteTZ.tz_name);
    strcat(sub, ": ");
    strcat(sub, RemoteTZ.tz_offset);
    menu_cell_basic_draw(ctx, cell_layer, "Change Timezone", sub, NULL);
    break;
  case 1 :
    if (RemoteTZ.tz_dst) {
      strcpy(sub, "On");
    } else {
      strcpy(sub, "Off");
    }
    menu_cell_basic_draw(ctx, cell_layer, "Daylight Savings", sub, NULL);
    break;
  }
}

void root_menu_select_callback(MenuLayer *me, MenuIndex *cell_index,
			       void *data) {
  switch (cell_index->row) {
  case 0:
    window_stack_push(&region_window, true);
  }
}

uint16_t region_menu_get_num_rows_callback(MenuLayer *me,
					 uint16_t section_index, void *data) {
  return NUM_REGIONS;
}

void region_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer,
				 MenuIndex *cell_index, void *data) {
  menu_cell_basic_draw(ctx, cell_layer, regions[cell_index->row], NULL, NULL);
}

void region_menu_select_callback(MenuLayer *me, MenuIndex *cell_index,
				 void *data) {
  // reindex for the selected zone
  read_file(cell_index->row+1);
  // display the zone window
  window_stack_push(&zone_window, true);
}

uint16_t zone_menu_get_num_rows_callback(MenuLayer *me,
					 uint16_t section_index, void *data) {
  return tz_count;
}

void zone_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer,
				   MenuIndex *cell_index, void *data) {

  fetch_time_zone(cell_index->row);
  menu_cell_basic_draw(ctx, cell_layer, SelectedTZ.tz_name,
		       SelectedTZ.tz_offset, NULL);
}

void zone_window_appear_handler(struct Window *window) {
  // Required because I'm changing the menu data on every appearance.
  menu_layer_reload_data(&zone_menu);
}

void handle_init(AppContextRef ctx) {
  resource_init_current_app(&APP_RESOURCES);

  // Setup for root window
  window_init(&root_window, "Dual-TZ configurator");
  menu_layer_init(&root_menu, root_window.layer.bounds);
  menu_layer_set_callbacks(&root_menu, NULL, (MenuLayerCallbacks){
      .get_num_rows = root_menu_get_num_rows_callback,
	.draw_row = root_menu_draw_row_callback,
	.select_click = root_menu_select_callback
	});
  menu_layer_set_click_config_onto_window(&root_menu, &root_window);
  layer_add_child(&root_window.layer, menu_layer_get_layer(&root_menu));

  // Setup for region window
  window_init(&region_window, "Region select");
  menu_layer_init(&region_menu, region_window.layer.bounds);
  menu_layer_set_callbacks(&region_menu, NULL, (MenuLayerCallbacks){
      .get_num_rows = region_menu_get_num_rows_callback,
	.draw_row = region_menu_draw_row_callback,
	.select_click = region_menu_select_callback
	 });
  menu_layer_set_click_config_onto_window(&region_menu, &region_window);
  layer_add_child(&region_window.layer, menu_layer_get_layer(&region_menu));

  // Setup for zone window
  window_init(&zone_window, "Zone select");
  window_set_window_handlers(&zone_window, (WindowHandlers) {
      .appear = &zone_window_appear_handler
	});
  menu_layer_init(&zone_menu, zone_window.layer.bounds);
  menu_layer_set_callbacks(&zone_menu, NULL, (MenuLayerCallbacks){
      .get_num_rows = zone_menu_get_num_rows_callback,
	.draw_row = zone_menu_draw_row_callback,
	});
  menu_layer_set_click_config_onto_window(&zone_menu, &zone_window);
  layer_add_child(&zone_window.layer, menu_layer_get_layer(&zone_menu));

  // Push root window to bottom of stack
  window_stack_push(&root_window, true /* Animated */);

  // Prime RemoteTZ with informative values
  strcpy(RemoteTZ.tz_name, "Retrieving...");
  strcpy(RemoteTZ.tz_offset, "???");

  // Populate the regions array
  regions[0] = "Africa";
  regions[1] = "America";
  regions[2] = "Antarctica";
  regions[3] = "Arctic";
  regions[4] = "Asia";
  regions[5] = "Atlantic";
  regions[6] = "Australia";
  regions[7] = "Europe";
  regions[8] = "Indian";
  regions[9] = "Pacific";
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
