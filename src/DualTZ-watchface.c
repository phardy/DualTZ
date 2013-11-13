#include <pebble.h>

#include "PDutils.h"

#include "../../common/config.h"
#include "common.h"

#ifdef ANDROID
PBL_APP_INFO(WATCHFACE_APP_UUID,
             "DualTZ", "Kids, Inc.",
             2, 0, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);
#else
PBL_APP_INFO(HTTP_UUID,
	     "DualTZ", "Kids, Inc.",
	     2, 0, /* App version */
	     RESOURCE_ID_IMAGE_MENU_ICON,
	     APP_INFO_WATCH_FACE);
#endif

// Layout stuff
Window window;
BmpContainer DigitalWindow;
GRect AnalogueGRect;
Layer *AnalogueHourLayer, *AnalogueMinuteLayer;
static GPath *AnalogueHourPath, *AnalogueMinutePath;
TextLayer TZName;
TextLayer TZOffset;
TextLayer DigitalTime;
TextLayer DigitalTimeS;
TextLayer Date;
TextLayer AmPm;
TextLayer FaceLabel;
static GFont TZFont;
static GFont DigitalTimeFont;
static GFont DigitalTimeSFont;
static GFont DateFont;

static TZInfo DisplayTZ;
static char DigitalTimeText[] = "  :  ";
static char DigitalTimeSText[] = "  ";
static char DateText[] = "  ";
static char DigitalTZOffset[] = "      ";
static char *DigitalTimeFormat;
TZState DigitalTZState = local;
int32_t localTZOffset = 0;

static const GPathInfo HOUR_HAND_PATH_POINTS = {
  .num_points = 5,
  .points = (GPoint[]) {
    {-4, 2},
    {4, 2},
    {4, -30},
    {0, -40},
    {-4, -30}
  }
};

static const GPathInfo MINUTE_HAND_PATH_POINTS = {
  .num_points = 5,
  .points = (GPoint []) {
    {-4, 2},
    {4, 2},
    {4, -50},
    {0, -60},
    {-4, -50}
  }
};

void update_digital_time(PblTm *time) {
  time_t t1 = mktime(time);
  int32_t t = (int32_t)t1 + localTZOffset;

  PblTm *adjTime;
  adjTime = gmtime(&t);
  string_format_time(DigitalTimeText, sizeof(DigitalTimeText),
		     DigitalTimeFormat, adjTime);
  if (!clock_is_24h_style()) {
    // Remove leading zero by overwriting it with a space
    if (DigitalTimeText[0] == '0') {
      DigitalTimeText[0] = ' ';
    }
    if (adjTime->tm_hour < 12) {
      // Nothing at all for AM
      text_layer_set_text(&AmPm, "  ");
    } else {
      text_layer_set_text(&AmPm, "PM");
    }
  }
  text_layer_set_text(&DigitalTime, DigitalTimeText);
}

void http_cookie_failed_callback(int32_t cookie, int http_status,
			      void* context) {
  // Bam. If we successfully got UTC time, may as well show it.
  if (DigitalTZState == utc) {
    text_layer_set_text(&TZName, "UTC (error)");
    text_layer_set_text(&TZOffset, "+0");

    PblTm now;
    get_time(&now);
    update_digital_time(&now);
  }
}

void http_cookie_get_callback (int32_t request_id, Tuple* result,
			       void* context) {
  if (request_id != HTTP_TZINFO_GET_REQ) return;
  if (result->key == HTTP_COOKIE_TZINFO) {
    parse_timezone((char *)result->value, &DisplayTZ);

    // Start displaying the TZ we just received
    DigitalTZState = remote;
    format_timezone(&DisplayTZ, DigitalTZOffset);
    text_layer_set_text(&TZName, DisplayTZ.tz_name);
    text_layer_set_text(&TZOffset, DigitalTZOffset);

    localTZOffset = localTZOffset + (DisplayTZ.tz_hours * 3600);
    if (DisplayTZ.tz_hours < 0) {
      localTZOffset = localTZOffset - (DisplayTZ.tz_minutes * 60);
    } else {
      localTZOffset = localTZOffset + (DisplayTZ.tz_minutes * 60);
    }
    PblTm now;
    get_time(&now);
    update_digital_time(&now);
  }
}
    
void http_time_callback (int32_t utc_offset_seconds, bool is_dst,
			 uint32_t unixtime, const char* tz_name,
			 void* context) {
  DigitalTZState = utc;
  localTZOffset =  -utc_offset_seconds;
  text_layer_set_text(&TZName, "UTC (fetching)");
  text_layer_set_text(&TZOffset, "+0");

  http_cookie_get(HTTP_TZINFO_GET_REQ, HTTP_COOKIE_TZINFO);
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
  window = window_create();
  window_stack_push(window, true /* Animated */);
  GRect window_bounds = layer_get_bounds(window_get_root_layer(window));

  // main time display
  DigitalTime = text_layer_create(GRect(17, 127, 90, 40)); // Size guess!
  text_layer_set_text_alignment(DigitalTime, GTextAlignmentRight);
  text_layer_set_text_color(DigitalTime, GColorBlack);
  text_layer_set_font(DigitalTime, DigitalTimeFont);
  layer_add_child(window_get_root_layer(window),
		  text_layer_get_layer(DigitalTime));

  // seconds display
  DigitalTimeS = text_layer_create(GRect(109, 147, 20, 20)); // sizing made me cry
  text_layer_set_text_alignment(DigitalTimeS, GTextAlignmentLeft);
  text_layer_set_text_color(DigitalTimeS, GColorBlack);
  text_layer_set_font(DigitalTimeS, DigitalTimeSFont);
  layer_add_child(window_get_root_layer(window), 
		  text_layer_get_layer(DigitalTimeS));

  // timezone name display
  TZName = text_layer_create(GRect(14, 125, 95, 15));
  text_layer_set_text_alignment(TZName, GTextAlignmentCenter);
  text_layer_set_text_color(TZName, GColorBlack);
  text_layer_set_font(TZName, TZFont);
  layer_add_child(window_get_root_layer(window),
		  text_layer_get_layer(TZName));

  // timezone offset display
  TZOffset = text_layer_create(GRect(110, 125, 40, 15));
  text_layer_set_text_alignment(TZOffset, GTextAlignmentLeft);
  text_layer_set_text_color(TZOffset, GColorBlack);
  text_layer_set_font(TZOffset, TZFont);
  layer_add_child(window_get_root_layer(window),
		  text_layer_get_layer(TZOffset));

  // date display
  Date = text_layer_create(GRect(112, 55, 20, 20));
  text_layer_set_text_alignment(Date, GTextAlignmentLeft);
  text_layer_set_text_color(Date, GColorBlack);
  text_layer_set_font(Date, DateFont);
  layer_add_child(window_get_root_layer(window),
		  text_layer_get_layer(Date));

  // AM/PM display
  if (!clock_is_24h_style()) {
    AmPm = text_layer_create(GRect(7, 153, 20, 20));
    text_layer_set_text_alignment(AmPm, GTextAlignmentLeft);
    text_layer_set_text_color(AmPm, GTextAlignmentLeft);
    text_layer_set_font(AmPm, TZFont);
    layer_add_child(window_get_root_layer(window),
		    text_layer_get_layer(AmPm));
  }

  // load background image
  // (it goes over the top to prevent clipping that was happening in old SDK)
  DigitalWindow = bitmap_layer_create(window_bounds);
  // TODO: Make sure this resource is the actual GBitmap.
  bitmap_layer_set_bitmap(DigitalWindow, RESOURCE_ID_IMAGE_DIGITAL_BG);
  bitmap_layer_set_compositing_mode(DigitalWindow, GCompOpAnd);
  layer_add_child(window_get_root_layer(window),
		  bitmap_layer_get_layer(DigitalWindow));

  // static face stuff
  FaceLabel = text_layer_create(GRect(52, 8, 40, 15));
  text_layer_set_text_alignment(FaceLabel, GTextAlignmentCenter);
  text_layer_set_background_color(FaceLabel, GColorBlack);
  text_layer_set_text_color(FaceLabel, GColorWhite);
  text_layer_set_font(FaceLabel, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(FaceLabel, "local");
  layer_add_chld(window_get_root_layer(window),
		 text_layer_get_layer(FaceLabel));

  // init analogue hands
  AnalogueMinuteLayer = layer_create(AnalogueGRect);
  layer_set_update_proc(AnalogueMinuteLayer, minute_display_layer_update_callback);
  AnalogueMinutePath = gpath_create(MINUTE_HAND_PATH_POINTS);
  gpath_move_to(AnalogueMinutePath,
		grect_center_point(AnalogueGRect));

  AnalogueHourLayer = layer_create(AnalogueGRect);
  layer_set_update_proc(AnalogueHourLayer, hour_display_layer_update_callback);
  AnalogueHourPath = gpath_create(HOUR_HAND_PATH_POINTS);
  gpath_move_to(AnalogueHourPath,
		grect_center_point(AnalogueGRect));
  layer_add_child(window_get_root_layer(window), AnalogueMinuteLayer);
  layer_add_child(window_get_root_layer(window), AnalogueHourLayer);
}

void handle_init() {
  // Math says a 128px box should be offset 8 pixels to be centred
  // in a 144px display. Pebble seems to say otherwise, though.
  AnalogueGRect = GRect(4, 0, 128, 128);
  display_init();

  strcpy(DisplayTZ.tz_name, "UTC");
  DisplayTZ.tz_hours = 0;
  DisplayTZ.tz_minutes = 0;

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
  text_layer_set_text(&Date, DateText);  // draw analogue hands
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

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}

handle_deinit() {
  gpath_destroy(AnalogueHourPath);
  layer_destroy(AnalogueHourLayer);
  gpath_destroy(AnalogueMinutePath);
  layer_destroy(AnalogueMinuteLayer);
  text_layer_destroy(FaceLabel);
  bitmap_layer_destroy(DigitalWindow);
  if (!clock_is_24h_style()) {
    text_layer_destroy(AmPm);
  text_layer_destroy(Date);
  text_layer_destroy(TZOffset);
  text_layer_destroy(TZName);
  text_layer_destroy(DigitalTimeS);
  text_layer_destroy(DigitalTime);
  window_destroy(window);
}

int main() {
  handle_init();
  app_event_loop();
  handle_deinit();
}
