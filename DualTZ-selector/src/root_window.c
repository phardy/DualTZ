#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "root_window.h"

uint16_t root_menu_get_num_rows_callback(MenuLayer *me,
					 uint16_t section_index, void *data) {
  return NUM_REGIONS;
}

void root_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer,
				 MenuIndex *cell_index, void *data) {
  menu_cell_basic_draw(ctx, cell_layer, regions[cell_index->row], NULL, NULL);
}

void root_window_load(Window *me) {
  GRect bounds = me->layer.bounds;

  menu_layer_init(&root_menu, bounds);

  MenuLayerCallbacks root_callbacks = {
    .get_num_rows = root_menu_get_num_rows_callback,
    .draw_row = root_menu_draw_row_callback,
    // .select_click = root_menu_select_callback
  };

  menu_layer_set_callbacks(&root_menu, NULL, root_callbacks);
  menu_layer_set_click_config_onto_window(&root_menu, me);
  layer_add_child(&me->layer, menu_layer_get_layer(&root_menu));
}
