#define DEBUG 1

#include <pebble.h>

#include "PDutils.h"
#include "tz.h"

#ifdef DEBUG
#include "debug.h"
#endif

// Layout stuff
Window *window;
GRect AnalogueGRect;
BitmapLayer *DigitalWindow;
Layer *AnalogueHourLayer, *AnalogueMinuteLayer;
static GPath *AnalogueHourPath, *AnalogueMinutePath;
static GBitmap *Background;
TextLayer *TZName;
TextLayer *TZOffset;
TextLayer *DigitalTime;
TextLayer *DigitalTimeS;
TextLayer *Date;
TextLayer *AmPm;
TextLayer *FaceLabel;
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

// data received from the config page
enum {
  CONFIG_KEY_REMOTE_TZ_NAME = 0x5D,
  CONFIG_KEY_REMOTE_TZ_OFFSET = 0x5E,
  CONFIG_KEY_LOCAL_TZ_OFFSET = 0x5F
};

void update_digital_time(struct tm *time) {
  time_t t1 = p_mktime(time);
  int32_t t = (int32_t)t1 + localTZOffset;

  struct tm *adjTime;
  adjTime = gmtime(&t);
  strftime(DigitalTimeText, sizeof(DigitalTimeText),
	   DigitalTimeFormat, adjTime);
  text_layer_set_text(DigitalTime, DigitalTimeText);
}

void in_dropped_handler(AppMessageResult reason, void *context) {
  // stub. request again?
}

void apply_stored_config() {
  if (persist_exists(CONFIG_KEY_REMOTE_TZ_NAME)) {
    persist_read_string(CONFIG_KEY_REMOTE_TZ_NAME,
			DisplayTZ.tz_name, TZ_NAME_LEN);
    DisplayTZ.tz_name[TZ_NAME_LEN] = '\0';
  } else {
    strncpy(DisplayTZ.tz_name, "local time", TZ_NAME_LEN);
  }
  text_layer_set_text(TZName, DisplayTZ.tz_name);
  if (persist_exists(CONFIG_KEY_REMOTE_TZ_OFFSET)) {
    DisplayTZ.tz_offset = persist_read_int(CONFIG_KEY_REMOTE_TZ_OFFSET);
    format_timezone(&DisplayTZ, DigitalTZOffset);
    text_layer_set_text(TZOffset, DigitalTZOffset);
  } else {
    // Don't write a timezone to the display here.
    DisplayTZ.tz_offset = 0;
  }
  if (persist_exists(CONFIG_KEY_LOCAL_TZ_OFFSET)) {
    localTZOffset = DisplayTZ.tz_offset - 
      persist_read_int(CONFIG_KEY_LOCAL_TZ_OFFSET);
  } else {
    localTZOffset = 0;
  }

  time_t t = time(NULL);
  struct tm *now;
  now = localtime(&t);
  update_digital_time(now);
}

void in_received_handler(DictionaryIterator *received, void *context) {
  Tuple *remote_tz_name_tuple = dict_find(received, CONFIG_KEY_REMOTE_TZ_NAME);
  Tuple *remote_tz_offset_tuple = dict_find(received, CONFIG_KEY_REMOTE_TZ_OFFSET);
  Tuple *local_tz_offset_tuple = dict_find(received, CONFIG_KEY_LOCAL_TZ_OFFSET);

  // Right now we only ever get all three in one packet
  if (remote_tz_name_tuple && remote_tz_offset_tuple && local_tz_offset_tuple) {
    int remote_tz_name_write = persist_write_string(CONFIG_KEY_REMOTE_TZ_NAME,
						    remote_tz_name_tuple->value->cstring);
    int remote_tz_offset_write = persist_write_int(CONFIG_KEY_REMOTE_TZ_OFFSET,
						   remote_tz_offset_tuple->value->int32);
    int local_tz_offset_write = persist_write_int(CONFIG_KEY_LOCAL_TZ_OFFSET,
						  local_tz_offset_tuple->value->int32);
#ifdef DEBUG
    debug_storage_write(remote_tz_name_write);
    debug_storage_write(remote_tz_offset_write);
    debug_storage_write(local_tz_offset_write);
#endif

    apply_stored_config();
  }
}

void hour_display_layer_update_callback (Layer *me, GContext* ctx) {
  (void)me;
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  unsigned int angle = (t->tm_hour * 30) + (t->tm_min / 2);
  gpath_rotate_to(AnalogueHourPath, (TRIG_MAX_ANGLE / 360) * angle);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, AnalogueHourPath);
  gpath_draw_outline(ctx, AnalogueHourPath);

  GRect my_frame = layer_get_frame(me);
  GPoint my_centre = grect_center_point(&my_frame);
  graphics_fill_circle(ctx, my_centre, 6);
  graphics_draw_circle(ctx, my_centre, 6);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, my_centre, 1);
}

void minute_display_layer_update_callback (Layer *me, GContext* ctx) {
  (void)me;
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  unsigned int angle = (t->tm_min * 6) + (t->tm_sec / 10);
  gpath_rotate_to(AnalogueMinutePath, (TRIG_MAX_ANGLE / 360) * angle);

  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  gpath_draw_filled(ctx, AnalogueMinutePath);
  gpath_draw_outline(ctx, AnalogueMinutePath);

  GRect my_frame = layer_get_frame(me);
  GPoint my_centre = grect_center_point(&my_frame);
  graphics_fill_circle(ctx, my_centre, 6);
  graphics_draw_circle(ctx, my_centre, 6);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, my_centre, 1);
}

void handle_second_tick(struct tm *now, TimeUnits units_changed) {
  strftime(DigitalTimeSText, sizeof(DigitalTimeSText),
	   "%S", now);
  text_layer_set_text(DigitalTimeS, DigitalTimeSText);

  if (now->tm_sec % 30 == 0) {
    layer_mark_dirty(AnalogueMinuteLayer);
    if (now->tm_min % 2 == 0) {
      layer_mark_dirty(AnalogueHourLayer);
    }

    update_digital_time(now);
  }
  if (now->tm_hour == 0 && now->tm_min == 0) {
    strftime(DateText, sizeof(DateText),
	     "%e", now);
    text_layer_set_text(Date, DateText);
  }
}

void display_init() {
  // load resources
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
  Background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DIGITAL_BG);
  bitmap_layer_set_bitmap(DigitalWindow, Background);
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
  layer_add_child(window_get_root_layer(window),
		 text_layer_get_layer(FaceLabel));

  // init analogue hands
  AnalogueMinuteLayer = layer_create(AnalogueGRect);
  layer_set_update_proc(AnalogueMinuteLayer, minute_display_layer_update_callback);
  AnalogueMinutePath = gpath_create(&MINUTE_HAND_PATH_POINTS);
  gpath_move_to(AnalogueMinutePath,
		grect_center_point(&AnalogueGRect));

  AnalogueHourLayer = layer_create(AnalogueGRect);
  layer_set_update_proc(AnalogueHourLayer, hour_display_layer_update_callback);
  AnalogueHourPath = gpath_create(&HOUR_HAND_PATH_POINTS);
  gpath_move_to(AnalogueHourPath,
		grect_center_point(&AnalogueGRect));
  layer_add_child(window_get_root_layer(window), AnalogueMinuteLayer);
  layer_add_child(window_get_root_layer(window), AnalogueHourLayer);
}

void handle_init() {
  // Math says a 128px box should be offset 8 pixels to be centred
  // in a 144px display. Pebble seems to say otherwise, though.
  AnalogueGRect = GRect(4, 0, 128, 128);
  display_init();

  strcpy(DisplayTZ.tz_name, "UTC");
  DisplayTZ.tz_offset = 0;

  if (clock_is_24h_style()) {
    DigitalTimeFormat = "%H:%M";
  } else {
    DigitalTimeFormat = "%l:%M";
  }

  // write current time to display
  apply_stored_config();
  time_t t = time(NULL);
  struct tm *now = localtime(&t);
  strftime(DateText, sizeof(DateText), "%e", now);
  text_layer_set_text(Date, DateText);  // draw analogue hands
  layer_mark_dirty(AnalogueMinuteLayer);
  layer_mark_dirty(AnalogueHourLayer);

  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  const uint32_t inbound_size = 64;
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
}

void handle_deinit() {
  gpath_destroy(AnalogueHourPath);
  layer_destroy(AnalogueHourLayer);
  gpath_destroy(AnalogueMinutePath);
  layer_destroy(AnalogueMinuteLayer);
  text_layer_destroy(FaceLabel);
  bitmap_layer_destroy(DigitalWindow);
  if (!clock_is_24h_style()) {
    text_layer_destroy(AmPm);
  }
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
