#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "http.h"

#include "Dual-TZ.h"
#include "ptime.h"
#include "config.h"

#define MY_UUID { 0x33, 0x1D, 0x7F, 0x32, 0x4F, 0xEE, 0x4D, 0x6C, 0xBD, 0x95, 0xE2, 0x7C, 0x6C, 0xDB, 0x44, 0x73 }
#define HTTP_APP_ID 5887304

PBL_APP_INFO(HTTP_UUID,
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
TextLayer DigitalTime;
TextLayer DigitalTimeS;
TextLayer Date;
TextLayer FaceLabel;
static GFont TZFont;
static GFont DigitalTimeFont;
static GFont DigitalTimeSFont;
static GFont DateFont;

// Time rememberating stuff.
// Currently hardcoded. Sorry.
char TZNameText[] = TZNAMETEXT;
char TZOffsetText[] = TZOFFSETTEXT;
int32_t TZOffsetS = TZOFFSETSEC;
static char DigitalTimeText[] = "00:00";
static char DigitalTimeSText[] = "00";
static char DateText[] = "  ";
static char *DigitalTimeFormat;
bool localTZSet = false;
int32_t localTZOffset = 0;

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

void http_time_callback (int32_t utc_offset_seconds, bool is_dst,
			 uint32_t unixtime, const char* tz_name,
			 void* context) {
  localTZSet = true;
  localTZOffset = TZOffsetS - utc_offset_seconds;

  text_layer_set_text(&TZName, TZNameText);
  text_layer_set_text(&TZOffset, TZOffsetText);

  PblTm now;
  get_time(&now);
  update_digital_time(&now);
}

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

void update_digital_time(PblTm *time) {
  time_t t1 = pmktime(time);
  int32_t t = (int32_t)t1 + localTZOffset;

  PblTm adjTime = plocaltime(&t);
  string_format_time(DigitalTimeText, sizeof(DigitalTimeText),
		     DigitalTimeFormat, &adjTime);
  text_layer_set_text(&DigitalTime, DigitalTimeText);
}

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {
  string_format_time(DigitalTimeSText, sizeof(DigitalTimeSText),
		     "%S", t->tick_time);
  text_layer_set_text(&DigitalTimeS, DigitalTimeSText);

  if (t->tick_time->tm_sec == 0) {
    layer_mark_dirty(&AnalogueMinuteLayer);
    if (t->tick_time->tm_min % 2 == 0) {
      layer_mark_dirty(&AnalogueHourLayer);
    }

    update_digital_time(t->tick_time);
  }
  if (t->tick_time->tm_hour == 0 && t->tick_time->tm_min == 0) {
    string_format_time(DateText, sizeof(DateText),
		       "%e", t->tick_time);
    text_layer_set_text(&Date, DateText);
  }
}

void display_init(AppContextRef *ctx) {
  // load resources
  resource_init_current_app(&APP_RESOURCES);
  TZFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_14));
  DigitalTimeFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_40));
  DigitalTimeSFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_20));
  DateFont = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);

  // init root window
  window_init(&window, "Root window");
  window_stack_push(&window, true /* Animated */);

  // main time display
  text_layer_init(&DigitalTime, GRect(17, 127, 90, 40)); // sizing hacky as hell
  text_layer_set_text_alignment(&DigitalTime, GTextAlignmentRight);
  text_layer_set_text_color(&DigitalTime, GColorBlack);
  text_layer_set_font(&DigitalTime, DigitalTimeFont);
  layer_add_child(&window.layer, &DigitalTime.layer);

  // seconds display
  text_layer_init(&DigitalTimeS, GRect(109, 147, 20, 20)); // sizing made me cry
  text_layer_set_text_alignment(&DigitalTimeS, GTextAlignmentLeft);
  text_layer_set_text_color(&DigitalTimeS, GColorBlack);
  text_layer_set_font(&DigitalTimeS, DigitalTimeSFont);
  layer_add_child(&window.layer, &DigitalTimeS.layer);

  // timezone name display
  text_layer_init(&TZName, GRect(14, 125, 95, 15));
  text_layer_set_text_alignment(&TZName, GTextAlignmentCenter);
  text_layer_set_text_color(&TZName, GColorBlack);
  text_layer_set_font(&TZName, TZFont);
  layer_add_child(&window.layer, &TZName.layer);

  // timezone offset display
  text_layer_init(&TZOffset, GRect(110, 125, 40, 15));
  text_layer_set_text_alignment(&TZOffset, GTextAlignmentLeft);
  text_layer_set_text_color(&TZOffset, GColorBlack);
  text_layer_set_font(&TZOffset, TZFont);
  layer_add_child(&window.layer, &TZOffset.layer);

  // date display
  text_layer_init(&Date, GRect(113, 55, 20, 20));
  text_layer_set_text_alignment(&Date, GTextAlignmentLeft);
  text_layer_set_text_color(&Date, GColorBlack);
  text_layer_set_font(&Date, DateFont);
  layer_add_child(&window.layer, &Date.layer);

  // load background image
  bmp_init_container(RESOURCE_ID_IMAGE_DIGITAL_BG, &DigitalWindow);
  bitmap_layer_set_compositing_mode(&DigitalWindow.layer, GCompOpAnd);
  layer_add_child(&window.layer, &DigitalWindow.layer.layer);

  // static face stuff
  text_layer_init(&FaceLabel, GRect(52, 8, 40, 15));
  text_layer_set_text_alignment(&FaceLabel, GTextAlignmentCenter);
  text_layer_set_background_color(&FaceLabel, GColorBlack);
  text_layer_set_text_color(&FaceLabel, GColorWhite);
  text_layer_set_font(&FaceLabel, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(&FaceLabel, "local");
  layer_add_child(&window.layer, &FaceLabel.layer);

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
    DigitalTimeFormat = "%H:%M";
  } else {
    DigitalTimeFormat = "%I:%M";
  }

  // write current time to display
  PblTm curTime;
  get_time(&curTime);
  string_format_time(DigitalTimeSText, sizeof(DigitalTimeSText),
		     "%S", &curTime);
  string_format_time(DigitalTimeText, sizeof(DigitalTimeText),
		     DigitalTimeFormat, &curTime);
  string_format_time(DateText, sizeof(DateText), 
		     "%e", &curTime);
  text_layer_set_text(&DigitalTime, DigitalTimeText);
  text_layer_set_text(&DigitalTimeS, DigitalTimeSText);
  // text_layer_set_text(&TZName, TZNameText);
  // text_layer_set_text(&TZOffset, TZOffsetText);
  text_layer_set_text(&TZName, "local time");
  text_layer_set_text(&TZOffset, " ");
  text_layer_set_text(&Date, DateText);

  // draw analogue hands
  layer_mark_dirty(&AnalogueMinuteLayer);
  layer_mark_dirty(&AnalogueHourLayer);

  http_set_app_id(HTTP_APP_ID);
  HTTPCallbacks callbacks = {
    .time = &http_time_callback
  };
  http_register_callbacks(callbacks, ctx);
  HTTPResult x = http_time_request();
  if (x == HTTP_BUSY) {
    text_layer_set_text(&TZName, "boom");
  }
}

void handle_deinit(AppContextRef ctx) {
  bmp_deinit_container(&DigitalWindow);
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,
    .messaging_info = {
      .buffer_sizes = {
	.inbound = 124,
	.outbound = 256
      }
    },
    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    }
  };
  app_event_loop(params, &handlers);
}
