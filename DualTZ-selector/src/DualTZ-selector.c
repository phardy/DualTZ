#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "http.h"
#include "PDutils.h"

#include "../../common/config.h"
#include "common.h"

#ifdef ANDROID
PBL_APP_INFO(SELECTOR_APP_UUID,
	     "DualTZ Config", "Kids, Inc.",
	     2, 0, /* App version */
	     RESOURCE_ID_IMAGE_MENU_ICON,
	     APP_INFO_STANDARD_APP);
#else
PBL_APP_INFO(HTTP_UUID,
	     "DualTZ Config", "KIDS, Inc.",
	     2, 0, /* App version */
	     RESOURCE_ID_IMAGE_MENU_ICON,
	     APP_INFO_STANDARD_APP);
#endif

#define BUF_SIZE 1024

// Longest file is 125, most are much shorter.
#define MAX_TZ 130

Window root_window;
Window region_window;
Window zone_window;
MenuLayer root_menu;
MenuLayer region_menu;
MenuLayer zone_menu;

// SelectedTZ is only used by menu update callbacks
TZInfo SelectedTZ;
// RemoteTZ is expected to always reflect
// what we believe is stored on the watch
TZInfo RemoteTZ;
// stageTZ is a placeholder for updates either to/from watch
TZInfo stageTZ;

// Number of TZ regions
#define NUM_REGIONS 11
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
  RESOURCE_ID_PACIFIC_TZ,
  RESOURCE_ID_UTC_TZ
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
// current resource handle, parses it in to the given TZInfo struct.
void fetch_time_zone(uint16_t idx, TZInfo *tz) {
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

  parse_timezone((char*)line, tz);
}

uint16_t root_menu_get_num_rows_callback(MenuLayer *me,
					 uint16_t section_index, void *data) {
  return 2;
}

void root_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer,
				 MenuIndex *cell_index, void *data) {
  char tz[TZ_OFFSET_LEN+1];
  char sub[TZ_NAME_LEN+TZ_OFFSET_LEN+3]; // +3 so we can add 2-char sep
  switch (cell_index->row) {
  case 0 :
    format_timezone(&RemoteTZ, tz);
    strcpy(sub, RemoteTZ.tz_name);
    strcat(sub, ": ");
    strcat(sub, tz);
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
    break;
  case 1:
    if (RemoteTZ.tz_dst) {
      RemoteTZ.tz_dst = false;
      RemoteTZ.tz_hours--;
    } else {
      RemoteTZ.tz_dst = true;
      RemoteTZ.tz_hours++;
    }
    // RemoteTZ is canonical, so copy it in to stageTZ and go
    strcpy(stageTZ.tz_name, RemoteTZ.tz_name);
    stageTZ.tz_hours = RemoteTZ.tz_hours;
    stageTZ.tz_minutes = RemoteTZ.tz_minutes;
    stageTZ.tz_dst = RemoteTZ.tz_dst;
    // two tabs, max length of six for TZ offset, one DST char
    char cookiestr[sizeof(stageTZ.tz_name) + 9];
    int isdst;
    if (stageTZ.tz_dst) isdst = 1;
    else isdst = 0;
    snprintf(cookiestr, sizeof(stageTZ.tz_name)+9, "%s\t%i:%i\t%i",
    	     stageTZ.tz_name, stageTZ.tz_hours,
    	     stageTZ.tz_minutes, isdst);
    http_cookie_set_cstring(HTTP_TZINFO_SET_REQ, HTTP_COOKIE_TZINFO, cookiestr);
    menu_layer_reload_data(&root_menu);
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

  fetch_time_zone(cell_index->row, &SelectedTZ);
  char offset[TZ_OFFSET_LEN+1];
  format_timezone(&SelectedTZ, offset);
  menu_cell_basic_draw(ctx, cell_layer, SelectedTZ.tz_name,
		       offset, NULL);
}

void zone_menu_select_callback(MenuLayer *me, MenuIndex *cell_index,
			       void *data) {
  fetch_time_zone(cell_index->row, &stageTZ);
  // Resetting DST to off seems like the best way
  // to ensure a known state. And confuse me less.
  stageTZ.tz_dst = false;
  char cookiestr[sizeof(stageTZ.tz_name) + 9];
  snprintf(cookiestr, sizeof(stageTZ.tz_name)+9, "%s\t%i:%i\t%i",
	   stageTZ.tz_name, stageTZ.tz_hours,
	   stageTZ.tz_minutes, stageTZ.tz_dst);
  http_cookie_set_cstring(HTTP_TZINFO_SET_REQ, HTTP_COOKIE_TZINFO, cookiestr);
}

void zone_window_appear_handler(struct Window *window) {
  // Required because I'm changing the menu data on every appearance.
  menu_layer_reload_data(&zone_menu);
}

void http_cookie_failed_callback(int32_t cookie, int http_status,
				 void* context) {
  strcpy(RemoteTZ.tz_name, "Unknown failure");
  RemoteTZ.tz_hours = 0;
  RemoteTZ.tz_minutes = 0;
  menu_layer_reload_data(&root_menu);
}

void http_cookie_get_callback(int32_t request_id, Tuple* result,
			      void* context) {
  if (request_id != HTTP_TZINFO_GET_REQ) return;
  if (result->key == HTTP_COOKIE_TZINFO) {
    parse_timezone((char *)result->value, &RemoteTZ);
    menu_layer_reload_data(&root_menu);
  }
}

void http_cookie_set_callback(int32_t request_id, bool successful,
			      void* context) {
  if (successful) {
    strcpy(RemoteTZ.tz_name, stageTZ.tz_name);
    RemoteTZ.tz_hours = stageTZ.tz_hours;
    RemoteTZ.tz_minutes = stageTZ.tz_minutes;
    RemoteTZ.tz_dst = stageTZ.tz_dst;
  } else {
    strcpy(RemoteTZ.tz_name, "Send failed");
  }
  menu_layer_reload_data(&root_menu);
}

void handle_init(AppContextRef ctx) {
  resource_init_current_app(&APP_RESOURCES);

  // Setup for root window
  window_init(&root_window, "Dual-TZ configurator");
  menu_layer_init(&root_menu, GRect(0, 0, 144, 153));
  menu_layer_set_callbacks(&root_menu, NULL, (MenuLayerCallbacks){
      .get_num_rows = root_menu_get_num_rows_callback,
	.draw_row = root_menu_draw_row_callback,
	.select_click = root_menu_select_callback
	});
  menu_layer_set_click_config_onto_window(&root_menu, &root_window);
  layer_add_child(&root_window.layer, menu_layer_get_layer(&root_menu));

  // Setup for region window
  window_init(&region_window, "Region select");
  // Using a static size because we don't yet know window size.
  menu_layer_init(&region_menu, GRect(0, 0, 144, 153));
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
  // Using a static size because we don't yet know window size.
  menu_layer_init(&zone_menu, GRect(0, 0, 144, 153));
  menu_layer_set_callbacks(&zone_menu, NULL, (MenuLayerCallbacks){
      .get_num_rows = zone_menu_get_num_rows_callback,
	.draw_row = zone_menu_draw_row_callback,
	.select_click = zone_menu_select_callback
	});
  menu_layer_set_click_config_onto_window(&zone_menu, &zone_window);
  layer_add_child(&zone_window.layer, menu_layer_get_layer(&zone_menu));

  // Push root window to bottom of stack
  window_stack_push(&root_window, true /* Animated */);

  // Prime RemoteTZ with informative values
  strcpy(RemoteTZ.tz_name, "Retrieving...");
  RemoteTZ.tz_hours = 0;
  RemoteTZ.tz_minutes = 0;
  RemoteTZ.tz_dst = false;

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
  regions[10] = "UTC";

  http_set_app_id(HTTP_APP_ID);
  http_register_callbacks((HTTPCallbacks) {
      .failure = http_cookie_failed_callback,
	.cookie_get = http_cookie_get_callback,
	.cookie_set = http_cookie_set_callback
	}, ctx);
  http_cookie_get(HTTP_TZINFO_GET_REQ, HTTP_COOKIE_TZINFO);
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
