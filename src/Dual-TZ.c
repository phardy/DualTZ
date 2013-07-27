#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"


#define MY_UUID { 0x33, 0x1D, 0x7F, 0x32, 0x4F, 0xEE, 0x4D, 0x6C, 0xBD, 0x95, 0xE2, 0x7C, 0x6C, 0xDB, 0x44, 0x73 }
PBL_APP_INFO(MY_UUID,
             "Dual-TZ", "Kids, Inc.",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

// Layout stuff
Window window;
BmpContainer DigitalWindow;
GRect AnalogueGRect;
Layer AnalogueHourLayer, AnalogueMinuteLayer;
GPath AnalogueHourPath, AnalogueMinutePath;
TextLayer TZName;
TextLayer TZOffset;
TextLayer TZTime;
TextLayer TZTimeS;
static GFont TZFont;
static GFont TimeFont;
static GFont TimeSFont;

// Time rememberating stuff.
char TZNameText[] = "Time zone label"; // max 15 characters (arbitrary)
char TZOffsetText[] = "+10";
static char TZTimeText[] = "00:00";
static char TZTimeSText[] = "00";
static char *TZFormat;

const GPathInfo HOUR_HAND_PATH_POINTS = {
  5,
  (GPoint[]) {
    {-4, 2},
    {4, 2},
    {4, -30},
    {0, -40},
    {-4, -30}
  }
};

const GPathInfo MINUTE_HAND_PATH_POINTS = {
  5,
  (GPoint []) {
    {-4, 2},
    {4, 2},
    {4, -50},
    {0, -60},
    {-4, -50}
  }
};

void initLayerPathAndCenter (Layer *layer, GPath *path,
			     const GPathInfo *pathInfo,
			     const void *updateProc) {
  layer_init(layer, AnalogueGRect);
  layer->update_proc = updateProc;
  gpath_init(path, pathInfo);
  gpath_move_to(path, grect_center_point(&layer->frame));
}

void hour_display_layer_update_callback (Layer *me, GContext* ctx) {
  (void)me;

  PblTm t;
  get_time(&t);

  unsigned int angle = (t.tm_hour * 30) + (t.tm_min / 2);
  gpath_rotate_to(&AnalogueHourPath, (TRIG_MAX_ANGLE / 360) * angle);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, &AnalogueHourPath);
  gpath_draw_outline(ctx, &AnalogueHourPath);

  graphics_fill_circle(ctx, grect_center_point(&me->frame), 6);
  graphics_draw_circle(ctx, grect_center_point(&me->frame), 6);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, grect_center_point(&me->frame), 1);
}

void minute_display_layer_update_callback (Layer *me, GContext* ctx) {
  (void)me;

  PblTm t;
  get_time(&t);

  unsigned int angle = (t.tm_min * 6) + (t.tm_sec / 10);
  gpath_rotate_to(&AnalogueMinutePath, (TRIG_MAX_ANGLE / 360) * angle);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, &AnalogueMinutePath);
  gpath_draw_outline(ctx, &AnalogueMinutePath);

  graphics_fill_circle(ctx, grect_center_point(&me->frame), 6);
  graphics_draw_circle(ctx, grect_center_point(&me->frame), 6);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, grect_center_point(&me->frame), 1);
}

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {
  string_format_time(TZTimeSText, sizeof(TZTimeSText), "%S", t->tick_time);
  text_layer_set_text(&TZTimeS, TZTimeSText);

  if (t->tick_time->tm_sec == 0) {
    layer_mark_dirty(&AnalogueMinuteLayer);
    if (t->tick_time->tm_min % 2 == 0) {
      layer_mark_dirty(&AnalogueHourLayer);
    }
    string_format_time(TZTimeText, sizeof(TZTimeText), TZFormat, t->tick_time);
    text_layer_set_text(&TZTime, TZTimeText);
  }
}

void display_init(AppContextRef *ctx) {
  // load resources
  resource_init_current_app(&APP_RESOURCES);
  TZFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_12));
  TimeFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_40));
  TimeSFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_20));

  // init root window
  window_init(&window, "Root window");
  window_stack_push(&window, true /* Animated */);

  // main time display
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
  text_layer_init(&TZName, GRect(24, 127, 75, 13));
  text_layer_set_text_alignment(&TZName, GTextAlignmentCenter);
  text_layer_set_text_color(&TZName, GColorBlack);
  text_layer_set_font(&TZName, TZFont);
  layer_add_child(&window.layer, &TZName.layer);

  // timezone offset display
  text_layer_init(&TZOffset, GRect(110, 127, 17, 13));
  text_layer_set_text_alignment(&TZOffset, GTextAlignmentLeft);
  text_layer_set_text_color(&TZOffset, GColorBlack);
  text_layer_set_font(&TZOffset, TZFont);
  layer_add_child(&window.layer, &TZOffset.layer);

  // load background image
  bmp_init_container(RESOURCE_ID_IMAGE_DIGITAL_BG, &DigitalWindow);
  bitmap_layer_set_compositing_mode(&DigitalWindow.layer, GCompOpAnd);
  layer_add_child(&window.layer, &DigitalWindow.layer.layer);

  // init analogue hands
  initLayerPathAndCenter(&AnalogueMinuteLayer, &AnalogueMinutePath,
			 &MINUTE_HAND_PATH_POINTS,
			 &minute_display_layer_update_callback);
  initLayerPathAndCenter(&AnalogueHourLayer, &AnalogueHourPath,
			 &HOUR_HAND_PATH_POINTS,
			 &hour_display_layer_update_callback);
  layer_add_child(&window.layer, &AnalogueMinuteLayer);
  layer_add_child(&window.layer, &AnalogueHourLayer);
}

void handle_init(AppContextRef ctx) {
  // Math says a 128px box should be offset 8 pixels to be centred
  // in a 144px display. Pebble seems to say otherwise, though.
  AnalogueGRect = GRect(4, 0, 128, 128);
  display_init(&ctx);

  if (clock_is_24h_style()) {
    TZFormat = "%H:%M";
  } else {
    TZFormat = "%I:%M";
  }

  // write current time to display
  PblTm curTime;
  get_time(&curTime);
  string_format_time(TZTimeSText, sizeof(TZTimeSText), "%S", &curTime);
  string_format_time(TZTimeText, sizeof(TZTimeText), TZFormat, &curTime);
  text_layer_set_text(&TZTime, TZTimeText);
  text_layer_set_text(&TZTimeS, TZTimeSText);
  text_layer_set_text(&TZName, TZNameText);
  text_layer_set_text(&TZOffset, TZOffsetText);

  // draw analogue hands
  layer_mark_dirty(&AnalogueMinuteLayer);
  layer_mark_dirty(&AnalogueHourLayer);
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
