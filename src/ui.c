#include <pebble.h>

const int TINY_NUMS[10] = {
  RESOURCE_ID_IMAGE_TINY_0, RESOURCE_ID_IMAGE_TINY_1, RESOURCE_ID_IMAGE_TINY_2,
  RESOURCE_ID_IMAGE_TINY_3, RESOURCE_ID_IMAGE_TINY_4, RESOURCE_ID_IMAGE_TINY_5,
  RESOURCE_ID_IMAGE_TINY_6, RESOURCE_ID_IMAGE_TINY_7, RESOURCE_ID_IMAGE_TINY_8,
  RESOURCE_ID_IMAGE_TINY_9};
const int MID_NUMS[10] = {
  RESOURCE_ID_IMAGE_MID_0, RESOURCE_ID_IMAGE_MID_1, RESOURCE_ID_IMAGE_MID_2,
  RESOURCE_ID_IMAGE_MID_3, RESOURCE_ID_IMAGE_MID_4, RESOURCE_ID_IMAGE_MID_5,
  RESOURCE_ID_IMAGE_MID_6, RESOURCE_ID_IMAGE_MID_7, RESOURCE_ID_IMAGE_MID_8,
  RESOURCE_ID_IMAGE_MID_9};
const int LARGE_NUMS[10] = {
  RESOURCE_ID_IMAGE_LARGE_0, RESOURCE_ID_IMAGE_LARGE_1,
  RESOURCE_ID_IMAGE_LARGE_2, RESOURCE_ID_IMAGE_LARGE_3,
  RESOURCE_ID_IMAGE_LARGE_4, RESOURCE_ID_IMAGE_LARGE_5,
  RESOURCE_ID_IMAGE_LARGE_6, RESOURCE_ID_IMAGE_LARGE_7,
  RESOURCE_ID_IMAGE_LARGE_8, RESOURCE_ID_IMAGE_LARGE_9};

#define TOTAL_IMAGE_SLOTS 6
#define EMPTY_SLOT -1
static int image_slot_state[TOTAL_IMAGE_SLOTS] = {EMPTY_SLOT, EMPTY_SLOT,
					      EMPTY_SLOT, EMPTY_SLOT,
					      EMPTY_SLOT, EMPTY_SLOT};
static GBitmap *DigitalTimeImages[TOTAL_IMAGE_SLOTS];
static BitmapLayer *DigitalTimeLayers[TOTAL_IMAGE_SLOTS];

Window *window;
GRect AnalogueGRect;
BitmapLayer *DigitalWindow;

Layer *AnalogueHourLayer, *AnalogueMinuteLayer;
static GPath *AnalogueHourPath, *AnalogueMinutePath;
static GBitmap *Background;
static BitmapLayer *ColonLayer;
TextLayer *TZName;
TextLayer *TZOffset;
TextLayer *Date;
TextLayer *AmPm;
TextLayer *FaceLabel;
static GFont TZFont;
static GFont DateFont;

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

static void load_digit_image_into_slot(int slot_number, uint32_t resource_id,
				       GRect frame) {
  /*

     Loads the digit image from the application's resources and
     displays it on-screen in the correct location.

     Each slot is a quarter of the screen.

   */

  // TODO: Signal these error(s)?

  if ((slot_number < 0) || (slot_number >= TOTAL_IMAGE_SLOTS)) {
    return;
  }

  if (image_slot_state[slot_number] != EMPTY_SLOT) {
    return;
  }
  image_slot_state[slot_number] = resource_id;
  DigitalTimeImages[slot_number] = gbitmap_create_with_resource(resource_id);

  BitmapLayer *bitmap_layer = bitmap_layer_create(frame);
  DigitalTimeLayers[slot_number] = bitmap_layer;
  bitmap_layer_set_bitmap(bitmap_layer, DigitalTimeImages[slot_number]);
  Layer *window_layer = window_get_root_layer(window);
  layer_add_child(window_layer, bitmap_layer_get_layer(bitmap_layer));
}

static void unload_digit_image_from_slot(int slot_number) {
  /*

     Removes the digit from the display and unloads the image resource
     to free up RAM.

     Can handle being called on an already empty slot.

   */

  if (image_slot_state[slot_number] != EMPTY_SLOT) {
    layer_remove_from_parent(bitmap_layer_get_layer(DigitalTimeLayers[slot_number]));
    bitmap_layer_destroy(DigitalTimeLayers[slot_number]);
    gbitmap_destroy(DigitalTimeImages[slot_number]);
    image_slot_state[slot_number] = EMPTY_SLOT;
  }

}

void load_image_into_slot(uint32_t resource, int slot) {
  GRect pos = layer_get_frame(bitmap_layer_get_layer(DigitalTimeLayers[slot]));

  unload_digit_image_from_slot(slot);
  load_digit_image_into_slot(slot, resource, pos);
}

void set_tzname_text(char *TZNameText) {
  text_layer_set_text(TZName, TZNameText);
}

void set_tzoffset_text(char *TZOffsetText) {
  text_layer_set_text(TZOffset, TZOffsetText);
}

void set_digital_text(struct tm *t) {
  int hour = t->tm_hour;
  int minute = t->tm_min;
  if (!clock_is_24h_style()) {
    if (hour > 12) {
      hour = hour - 12;
    }
    if (hour == 0) {
      hour = 12;
    }
  }
  int hourtens = hour / 10;
  int hourunits = hour % 10;
  if (clock_is_24h_style() || hourtens == 1) {
    load_image_into_slot(LARGE_NUMS[hourtens], 0);
  } else {
    load_image_into_slot(RESOURCE_ID_IMAGE_LARGE_BLANK, 0);
  }
  load_image_into_slot(LARGE_NUMS[hourunits], 1);
  int minutetens = minute / 10;
  load_image_into_slot(LARGE_NUMS[minutetens], 2);
  int minuteunits = minute % 10;
  load_image_into_slot(LARGE_NUMS[minuteunits], 3);

  if (!clock_is_24h_style() && t->tm_hour >= 12) {
    layer_remove_from_parent(text_layer_get_layer(AmPm));
    layer_add_child(window_get_root_layer(window),
		    text_layer_get_layer(AmPm));
  }
}

void set_digitals_text(struct tm *time) {
  int secondtens = time->tm_sec / 10;
  load_image_into_slot(MID_NUMS[secondtens], 4);
  int secondunits = time->tm_sec % 10;
  load_image_into_slot(MID_NUMS[secondunits], 5);
}

void set_date_text(char *DateText) {
  text_layer_set_text(Date, DateText);
}

void update_minute_hand() {
  layer_mark_dirty(AnalogueMinuteLayer);
}

void update_hour_hand() {
  layer_mark_dirty(AnalogueHourLayer);
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

void display_init() {
  // load resources
  TZFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_DIGITAL_14));
  DateFont = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);

  // Math says a 128px box should be offset 8 pixels to be centred
  // in a 144px display. Pebble seems to say otherwise, though.
  AnalogueGRect = GRect(4, 0, 128, 128);

  // init root window
  window = window_create();
  window_stack_push(window, true /* Animated */);
  GRect window_bounds = layer_get_bounds(window_get_root_layer(window));

  // main time display
  DigitalTimeLayers[0] = bitmap_layer_create(GRect(30, 141, 16, 26)); // no margin/padding yet
  DigitalTimeLayers[1] = bitmap_layer_create(GRect(48, 141, 16, 26));
  DigitalTimeLayers[2] = bitmap_layer_create(GRect(71, 141, 16, 26));
  DigitalTimeLayers[3] = bitmap_layer_create(GRect(90, 141, 16, 26));
  for (int i=0; i< 4; i++) {
    layer_add_child(window_get_root_layer(window),
		    bitmap_layer_get_layer(DigitalTimeLayers[i]));
  }
  ColonLayer = bitmap_layer_create(GRect(64, 141, 16, 26)); // TODO: Shrink this image
  GBitmap *colon;
  colon = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LARGE_COLON);
  bitmap_layer_set_bitmap(ColonLayer, colon);
  layer_add_child(window_get_root_layer(window),
		  bitmap_layer_get_layer(ColonLayer));

  // seconds display
  DigitalTimeLayers[4] = bitmap_layer_create(GRect(110, 153, 8, 14));
  DigitalTimeLayers[5] = bitmap_layer_create(GRect(120, 153, 8, 14));
  for (int i=4; i< 6; i++) {
    layer_add_child(window_get_root_layer(window),
		    bitmap_layer_get_layer(DigitalTimeLayers[i]));
  }

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

  // load background image
  // (it goes over the top to prevent clipping that was happening in old SDK)
  DigitalWindow = bitmap_layer_create(window_bounds);
  // TODO: Make sure this resource is the actual GBitmap.
  Background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DIGITAL_BG);
  bitmap_layer_set_bitmap(DigitalWindow, Background);
  bitmap_layer_set_compositing_mode(DigitalWindow, GCompOpAnd);
  layer_add_child(window_get_root_layer(window),
		  bitmap_layer_get_layer(DigitalWindow));
  // AM/PM display
  if (!clock_is_24h_style()) {
    AmPm = text_layer_create(GRect(20, 153, 20, 20));
    text_layer_set_text_alignment(AmPm, GTextAlignmentLeft);
    text_layer_set_text_color(AmPm, GTextAlignmentLeft);
    text_layer_set_font(AmPm, TZFont);
    text_layer_set_text(AmPm, "PM");
  }

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

void display_deinit() {
  gpath_destroy(AnalogueHourPath);
  layer_destroy(AnalogueHourLayer);
  gpath_destroy(AnalogueMinutePath);
  layer_destroy(AnalogueMinuteLayer);
  text_layer_destroy(FaceLabel);
  gbitmap_destroy(Background);
  bitmap_layer_destroy(DigitalWindow);
  if (!clock_is_24h_style()) {
    text_layer_destroy(AmPm);
  }
  bitmap_layer_destroy(ColonLayer);
  text_layer_destroy(Date);
  text_layer_destroy(TZOffset);
  text_layer_destroy(TZName);
  for (int i=0; i< 6; i++) {
    gbitmap_destroy(DigitalTimeImages[i]);
    bitmap_layer_destroy(DigitalTimeLayers[i]);
  }
  window_destroy(window);
}
