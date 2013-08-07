#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "../../common/config.h"

#ifdef ANDROID
PBL_APP_INFO(SELECTOR_APP_UUID,
	     "Dual-TZ Config", "Kids, Inc.",
	     1, 0, /* App version */
	     DEFAULT_MENU_ICON,
	     APP_INFO_STANDARD_APP);
#else
PBL_APP_INFO(HTTP_UUID,
	     "Dual-TZ Config", "KIDS, Inc.",
	     1, 0, /* App version */
	     DEFAULT_MENU_ICON,
	     APP_INFO_STANDARD_APP);
#endif

Window window;


void handle_init(AppContextRef ctx) {

  window_init(&window, "Window Name");
  window_stack_push(&window, true /* Animated */);

  resource_init_current_app(&APP_RESOURCES);
}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init
  };
  app_event_loop(params, &handlers);
}
