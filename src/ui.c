#include <pebble.h>

const int TINY_NUMS[10] = {
  RESOURCE_ID_IMAGE_TINY_0, RESOURCE_ID_IMAGE_TINY_1, RESOURCE_ID_IMAGE_TINY_2,
  RESOURCE_ID_IMAGE_TINY_3, RESOURCE_ID_IMAGE_TINY_4, RESOURCE_ID_IMAGE_TINY_5,
  RESOURCE_ID_IMAGE_TINY_6, RESOURCE_ID_IMAGE_TINY_7, RESOURCE_ID_IMAGE_TINY_8,
  RESOURCE_ID_IMAGE_TINY_9};

Window *window;
GRect AnalogueGRect;
BitmapLayer *DigitalWindow;

Layer *AnalogueHourLayer, *AnalogueMinuteLayer;
static GPath *AnalogueHourPath, *AnalogueMinutePath;
static GBitmap *Background;
static GBitmap *LARGE_NUMS, *MID_NUMS;
static GBitmap *DigitalTimeImages[6];
static BitmapLayer *DigitalTime[6];
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

void load_image_into_layer(GBitmap *source, GRect offset,
			   GBitmap *bitmap, BitmapLayer *layer) {

  GRect pos = layer_get_frame(bitmap_layer_get_layer(layer));
  layer_remove_from_parent(bitmap_layer_get_layer(layer));
  gbitmap_destroy(bitmap);
  bitmap_layer_destroy(layer);

  layer = bitmap_layer_create(pos);
  bitmap_layer_set_compositing_mode(layer, GCompOpAnd);
  bitmap = gbitmap_create_as_sub_bitmap(source, offset);
  bitmap_layer_set_bitmap(layer, bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(layer));
}

void set_tzname_text(char *TZNameText) {
  text_layer_set_text(TZName, TZNameText);
}

void set_tzoffset_text(char *TZOffsetText) {
  text_layer_set_text(TZOffset, TZOffsetText);
}

void set_digital_text(struct tm *time) {
  int hourtens, hourunits;
  if (clock_is_24h_style()) {
    hourtens = time->tm_hour / 10;
    hourunits = time->tm_hour % 10;
  } else {
    hourtens = (time->tm_hour % 12) /10;
    hourunits = (time->tm_hour % 12) % 10;
    if (time->tm_hour > 11) {
      text_layer_set_text(AmPm, "PM");
    } else {
      text_layer_set_text(AmPm, "");
    }
  }
  GRect hourtensnum;
  if (clock_is_24h_style() || hourtens == 1) {
    hourtensnum = GRect(16*hourtens, 0, 16, 26);
  } else {
    hourtensnum = GRect(176, 0, 16, 26);
  }
  load_image_into_layer(LARGE_NUMS, hourtensnum,
			DigitalTimeImages[0], DigitalTime[0]);
  load_image_into_layer(LARGE_NUMS, GRect(16*hourunits, 0, 16, 26),
			DigitalTimeImages[1], DigitalTime[1]);
  int minutetens = time->tm_min / 10;
  load_image_into_layer(LARGE_NUMS, GRect(16*minutetens, 0, 16, 26),
			DigitalTimeImages[2], DigitalTime[2]);
  int minuteunits = time->tm_min % 10;
  load_image_into_layer(LARGE_NUMS, GRect(16*minuteunits, 0, 16, 26),
			DigitalTimeImages[3], DigitalTime[3]);
}

void set_digitals_text(struct tm *time) {
  int secondtens = time->tm_sec / 10;
  load_image_into_layer(MID_NUMS, GRect(8*secondtens, 0, 8, 14),
			DigitalTimeImages[4], DigitalTime[4]);
  int secondunits = time->tm_sec % 10;
  load_image_into_layer(MID_NUMS, GRect(8*secondunits, 0, 8, 14),
			DigitalTimeImages[5], DigitalTime[5]);
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
  LARGE_NUMS = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LARGE_NUMS);
  MID_NUMS = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MID_NUMS);

  // Math says a 128px box should be offset 8 pixels to be centred
  // in a 144px display. Pebble seems to say otherwise, though.
  AnalogueGRect = GRect(4, 0, 128, 128);

  // init root window
  window = window_create();
  window_stack_push(window, true /* Animated */);
  GRect window_bounds = layer_get_bounds(window_get_root_layer(window));

  // main time display
  DigitalTime[0] = bitmap_layer_create(GRect(30, 141, 16, 26)); // no margin/padding yet
  DigitalTime[1] = bitmap_layer_create(GRect(48, 141, 16, 26));
  DigitalTime[2] = bitmap_layer_create(GRect(71, 141, 16, 26));
  DigitalTime[3] = bitmap_layer_create(GRect(90, 141, 16, 26));
  for (int i=0; i< 4; i++) {
    layer_add_child(window_get_root_layer(window),
		    bitmap_layer_get_layer(DigitalTime[i]));
    DigitalTimeImages[i] = gbitmap_create_as_sub_bitmap(LARGE_NUMS,
							 GRect(176, 0, 16, 26));
    bitmap_layer_set_bitmap(DigitalTime[i], DigitalTimeImages[i]);
  }
  ColonLayer = bitmap_layer_create(GRect(64, 141, 16, 26)); // TODO: Shrink this image
  GBitmap *colon;
  colon = gbitmap_create_as_sub_bitmap(LARGE_NUMS, GRect(160, 0, 16, 26));
  bitmap_layer_set_bitmap(ColonLayer, colon);
  layer_add_child(window_get_root_layer(window),
		  bitmap_layer_get_layer(ColonLayer));

  // seconds display
  DigitalTime[4] = bitmap_layer_create(GRect(110, 153, 8, 14));
  DigitalTime[5] = bitmap_layer_create(GRect(120, 153, 8, 14));
  for (int i=4; i< 6; i++) {
    layer_add_child(window_get_root_layer(window),
		    bitmap_layer_get_layer(DigitalTime[i]));
    DigitalTimeImages[i] = gbitmap_create_as_sub_bitmap(MID_NUMS,
							GRect(0, 0, 8, 14));
    bitmap_layer_set_bitmap(DigitalTime[i], DigitalTimeImages[i]);
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

  // AM/PM display
  if (!clock_is_24h_style()) {
    AmPm = text_layer_create(GRect(20, 153, 20, 20));
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

void display_deinit() {
  gpath_destroy(AnalogueHourPath);
  layer_destroy(AnalogueHourLayer);
  gpath_destroy(AnalogueMinutePath);
  layer_destroy(AnalogueMinuteLayer);
  text_layer_destroy(FaceLabel);
  gbitmap_destroy(Background);
  gbitmap_destroy(LARGE_NUMS);
  gbitmap_destroy(MID_NUMS);
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
    bitmap_layer_destroy(DigitalTime[i]);
  }
  window_destroy(window);
}
