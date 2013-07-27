#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0x33, 0x1D, 0x7F, 0x32, 0x4F, 0xEE, 0x4D, 0x6C, 0xBD, 0x95, 0xE2, 0x7C, 0x6C, 0xDB, 0x44, 0x73 }
PBL_APP_INFO(MY_UUID,
             "Dual-TZ", "Kids, Inc.",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;
BmpContainer DigitalWindow;
TextLayer TZName;
TextLayer TZOffset;
TextLayer TZTime;
TextLayer TZTimeS;
static GFont TZFont;
static GFont TimeFont;
static GFont TimeSFont;

// These are generic vars
// char TZNameText[] = "xxxxxxxxxxxxxxx"; // 15 characters
// char TZOffsetText[] = "+xx";
char TZNameText[] = "Time zone label";
char TZOffsetText[] = "+10";
static char TZTimeText[] = "00:00";
static char TZTimeSText[] = "00";
static char *TZFormat;

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {
  string_format_time(TZTimeSText, sizeof(TZTimeSText), "%S", t->tick_time);
  text_layer_set_text(&TZTimeS, TZTimeSText);

  if (t->tick_time->tm_sec == 0) {
    string_format_time(TZTimeText, sizeof(TZTimeText), TZFormat, t->tick_time);
    text_layer_set_text(&TZTime, TZTimeText);
  }
}

void display_init() {
  // load resources
  resource_init_current_app(&APP_RESOURCES);
  TZFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_12));
  TimeFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_40));
  TimeSFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_20));

  // init root window
  window_init(&window, "Root window");
  window_stack_push(&window, true /* Animated */);

  // load background image
  bmp_init_container(RESOURCE_ID_IMAGE_DIGITAL_BG, &DigitalWindow);
  layer_add_child(&window.layer, &DigitalWindow.layer.layer);

  // main time display
  // TODO: This box currently overlaps the top of the white
  // display background. Either shrink the box, or draw the black
  // bits transparent over the top.
  text_layer_init(&TZTime, GRect(17, 127, 90, 40)); // sizing hacky as hell
  text_layer_set_text_alignment(&TZTime, GTextAlignmentRight);
  text_layer_set_text_color(&TZTime, GColorBlack);
  text_layer_set_font(&TZTime, TimeFont);
  layer_add_child(&window.layer, &TZTime.layer);

  // seconds display
  text_layer_init(&TZTimeS, GRect(109, 147, 20, 20)); // sizing made me cry
  text_layer_set_text_alignment(&TZTimeS, GTextAlignmentLeft);
  text_layer_set_text_color(&TZTimeS, GColorBlack);
  text_layer_set_font(&TZTimeS, TimeSFont);
  layer_add_child(&window.layer, &TZTimeS.layer);

  // timezone name display
  text_layer_init(&TZName, GRect(1, 127, 120, 13));
  text_layer_set_text_alignment(&TZName, GTextAlignmentRight);
  text_layer_set_text_color(&TZName, GColorBlack);
  text_layer_set_font(&TZName, TZFont);
  layer_add_child(&window.layer, &TZName.layer);

  // timezone offset display
  text_layer_init(&TZOffset, GRect(120, 127, 24, 13));
  text_layer_set_text_alignment(&TZOffset, GTextAlignmentRight);
  text_layer_set_text_color(&TZOffset, GColorBlack);
  text_layer_set_font(&TZOffset, TZFont);
  layer_add_child(&window.layer, &TZOffset.layer);
}

void handle_init(AppContextRef ctx) {
  display_init();
  text_layer_set_text(&TZTime, TZTimeText);
  text_layer_set_text(&TZTimeS, TZTimeSText);
  text_layer_set_text(&TZName, TZNameText);
  text_layer_set_text(&TZOffset, TZOffsetText);

  if (clock_is_24h_style()) {
    TZFormat = "%H:%M";
  } else {
    TZFormat = "%I:%M";
  }
}

void handle_deinit(AppContextRef ctx) {
  bmp_deinit_container(&DigitalWindow);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,

    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
