#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "http.h"
#include "PDutils.h"

#include "DualTZ-watchface.h"
#include "../../common/config.h"

#ifdef ANDROID
PBL_APP_INFO(WATCHFACE_APP_UUID,
             "DualTZ", "Kids, Inc.",
             1, 2, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);
#else
PBL_APP_INFO(HTTP_UUID,
	     "DualTZ", "Kids, Inc.",
	     1, 2, /* App version */
	     RESOURCE_ID_IMAGE_MENU_ICON,
	     APP_INFO_WATCH_FACE);
#endif

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

static TZInfo DisplayTZ;
static char DigitalTimeText[] = "00:00";
static char DigitalTimeSText[] = "00";
static char DateText[] = "  ";
static char *DigitalTimeFormat;
TZState DigitalTZState = local;
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

void http_cookie_failed_callback(int32_t cookie, int http_status,
			      void* context) {
  // Bam. If we successfully got UTC time, may as well show it.
  if (DigitalTZState == utc) {
      text_layer_set_text(&TZName, UTC.tz_name);
      text_layer_set_text(&TZOffset, UTC.tz_offset);

      PblTm now;
      get_time(&now);
      update_digital_time(&now);
  }
}

void http_cookie_get_callback (int32_t request_id, Tuple* result,
			       void* context) {
  if (request_id != HTTP_TZINFO_GET_REQ) return;
  if (result->key == HTTP_COOKIE_TZINFO) {
    TZInfo *tmpTZ = (TZInfo *) result->value;
    strcpy(DisplayTZ.tz_name, tmpTZ->tz_name);
    strcpy(DisplayTZ.tz_offset, tmpTZ->tz_offset);
    DisplayTZ.tz_seconds = tmpTZ->tz_seconds;
    DisplayTZ.tz_dst = tmpTZ->tz_dst;

    // Start displaying the TZ we just received
    DigitalTZState = remote;
    localTZOffset = localTZOffset + DisplayTZ.tz_seconds;
    text_layer_set_text(&TZName, DisplayTZ.tz_name);
    text_layer_set_text(&TZOffset, DisplayTZ.tz_offset);

    PblTm now;
    get_time(&now);
    update_digital_time(&now);
  }
}
    
void http_time_callback (int32_t utc_offset_seconds, bool is_dst,
			 uint32_t unixtime, const char* tz_name,
			 void* context) {
  DigitalTZState = utc;
  localTZOffset = UTC.tz_seconds - utc_offset_seconds;

  // Don't update anything yet until we've
  // tried to request the remote TZ
  http_cookie_get(HTTP_TZINFO_GET_REQ, HTTP_COOKIE_TZINFO);
}

void init_layer_path_and_center (Layer *layer, GPath *path,
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

  PblTm *adjTime;
  adjTime = pgmtime(&t);
  string_format_time(DigitalTimeText, sizeof(DigitalTimeText),
		     DigitalTimeFormat, adjTime);
  text_layer_set_text(&DigitalTime, DigitalTimeText);
}

void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t) {
  string_format_time(DigitalTimeSText, sizeof(DigitalTimeSText),
		     "%S", t->tick_time);
  text_layer_set_text(&DigitalTimeS, DigitalTimeSText);

  if (t->tick_time->tm_sec % 30 == 0) {
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
  text_layer_init(&Date, GRect(112, 55, 20, 20));
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
  init_layer_path_and_center(&AnalogueMinuteLayer, &AnalogueMinutePath,
			 &MINUTE_HAND_PATH_POINTS,
			 &minute_display_layer_update_callback);
  init_layer_path_and_center(&AnalogueHourLayer, &AnalogueHourPath,
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

  strcpy(DisplayTZ.tz_name, UTC.tz_name);
  strcpy(DisplayTZ.tz_offset, UTC.tz_offset);
  DisplayTZ.tz_seconds = UTC.tz_seconds;

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
  text_layer_set_text(&TZName, "local time");
  text_layer_set_text(&TZOffset, " ");
  text_layer_set_text(&Date, DateText);

  // draw analogue hands
  layer_mark_dirty(&AnalogueMinuteLayer);
  layer_mark_dirty(&AnalogueHourLayer);

  http_set_app_id(HTTP_APP_ID);
  HTTPCallbacks callbacks = {
    .failure = http_cookie_failed_callback,
    .cookie_get = http_cookie_get_callback,
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
