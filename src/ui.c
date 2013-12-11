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
static GBitmap *ColonBitmap;
static GRect DigitalTimeDigits[TOTAL_IMAGE_SLOTS];
static Layer *DigitalTimeLayer;
struct tm *DigitalTime;

Window *window;
GRect AnalogueGRect;
BitmapLayer *DigitalWindow;

Layer *AnalogueHourLayer, *AnalogueMinuteLayer;
static GPath *AnalogueHourPath, *AnalogueMinutePath;
static GBitmap *Background;
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

void set_tzname_text(char *TZNameText) {
  text_layer_set_text(TZName, TZNameText);
}

void set_tzoffset_text(char *TZOffsetText) {
  text_layer_set_text(TZOffset, TZOffsetText);
}

void set_digital_time(struct tm *t) {
  DigitalTime = t;
  layer_mark_dirty(DigitalTimeLayer);
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

void update_digital_digit(int slot, int digit) {
  if (image_slot_state[slot] != digit) {
    if (image_slot_state[slot] != EMPTY_SLOT) {
      gbitmap_destroy(DigitalTimeImages[slot]);
    }
    // 0-3 are large, 4,5 are smaller seconds digits
    if (slot < 4) {
      DigitalTimeImages[slot] = gbitmap_create_with_resource(LARGE_NUMS[digit]);
    } else {
      DigitalTimeImages[slot] = gbitmap_create_with_resource(MID_NUMS[digit]);
    }
    image_slot_state[slot] = digit;
  }
}

void digital_layer_update_callback(Layer *me, GContext* ctx) {
  graphics_draw_bitmap_in_rect(ctx, ColonBitmap, GRect(69, 13, 16, 26));
  // Splitting the seconds layer off is probably more efficient.
  int hour = DigitalTime->tm_hour;
  int minute = DigitalTime->tm_min;
  int second = DigitalTime->tm_sec;
  int displaytime[TOTAL_IMAGE_SLOTS];
  if (!clock_is_24h_style()) {
    if (hour > 12) {
      hour = hour-12;
    }
    if (hour == 0) {
      hour = 12;
    }
  }
  displaytime[0] = hour / 10;
  displaytime[1] = hour % 10;
  displaytime[2] = minute / 10;
  displaytime[3] = minute % 10;
  displaytime[4] = second / 10;
  displaytime[5] = second % 10;

  if (clock_is_24h_style() || displaytime[0] == 1) {
    update_digital_digit(0, displaytime[0]);
    graphics_draw_bitmap_in_rect(ctx, DigitalTimeImages[0], DigitalTimeDigits[0]);
  } else {
    gbitmap_destroy(DigitalTimeImages[0]);
    image_slot_state[0] = EMPTY_SLOT;
  }

  for (int i=1; i<TOTAL_IMAGE_SLOTS; i++) {
    update_digital_digit(i, displaytime[i]);
    graphics_draw_bitmap_in_rect(ctx, DigitalTimeImages[i], DigitalTimeDigits[i]);
  }

  // TODO: Replace this with a bitmap?
  if (!clock_is_24h_style() && DigitalTime->tm_hour >= 12) {
    layer_remove_from_parent(text_layer_get_layer(AmPm));
    layer_add_child(window_get_root_layer(window),
		    text_layer_get_layer(AmPm));
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

  // digital time display
  DigitalTimeLayer = layer_create(GRect(0, 128, 144, 40));
  layer_set_update_proc(DigitalTimeLayer, digital_layer_update_callback);
  layer_add_child(window_get_root_layer(window), DigitalTimeLayer);
  DigitalTimeDigits[0] = GRect(35, 13, 16, 26);
  DigitalTimeDigits[1] = GRect(53, 13, 16, 26);
  DigitalTimeDigits[2] = GRect(76, 13, 16, 26);
  DigitalTimeDigits[3] = GRect(95, 13, 16, 26);
  DigitalTimeDigits[4] = GRect(115, 25, 8, 14);
  DigitalTimeDigits[5] = GRect(125, 25, 8, 14);

  ColonBitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LARGE_COLON);
  // TODO: Move to the update. But I want to remember this GRect
  // ColonLayer = bitmap_layer_create(GRect(69, 141, 16, 26)); // TODO: Shrink this image

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
  if (!clock_is_24h_style()) {
    text_layer_destroy(AmPm);
  }
  bitmap_layer_destroy(DigitalWindow);
  gbitmap_destroy(Background);
  text_layer_destroy(Date);
  text_layer_destroy(TZOffset);
  text_layer_destroy(TZName);
  for (int i=0; i< 6; i++) {
    if (image_slot_state[i] != EMPTY_SLOT) {
      gbitmap_destroy(DigitalTimeImages[i]);
    }
  }
  gbitmap_destroy(ColonBitmap);
  layer_destroy(DigitalTimeLayer);
  window_destroy(window);
  fonts_unload_custom_font(TZFont);
}
