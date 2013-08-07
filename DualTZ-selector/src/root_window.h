#include "DualTZ-selector.h"

void root_window_load(Window *me);
void root_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer,
				 MenuIndex *cell_index, void *data);
uint16_t root_menu_get_num_rows_callback(MenuLayer *me,
					 uint16_t section_index, void *data);
