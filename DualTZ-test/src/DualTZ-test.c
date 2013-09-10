#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "http.h"
#include "xprintf.h"

#include "../../../common/config.h"
#include "common.h"

#ifdef ANDROID
#define MY_UUID { 0x55, 0x62, 0x4E, 0x7B, 0xFA, 0x8B, 0x47, 0xD7, 0x9E, 0x9E, 0xFE, 0xC2, 0xA2, 0xB1, 0xB4, 0xA5 }
PBL_APP_INFO(MY_UUID,
             "DualTZ Test", "Kids, Inc.",
             2, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);
#else
PBL_APP_INFO(HTTP_UUID,
	     "DualTZ Test", "Kids, Inc.",
	     2, 0, /* App version */
	     DEFAULT_MENU_ICON,
	     APP_INFO_STANDARD_APP);
#endif

Window window;
TextLayer textlayer;

char outputstr[128];

void http_cookie_failed_callback(int32_t cookie, int http_status,
				 void* context) {
  http_status = http_status - 1000;
  if (http_status == HTTP_OK) {
    strcpy(outputstr, "HTTP_OK");
  } else if (http_status == HTTP_SEND_TIMEOUT) {
    strcpy(outputstr, "HTTP_SEND_TIMEOUT");
  } else if (http_status == HTTP_BRIDGE_NOT_RUNNING) {
    strcpy(outputstr, "HTTP_BRIDGE_NOT_RUNNING");
  } else if (http_status == HTTP_INVALID_ARGS) {
    strcpy(outputstr, "HTTP_INVALID_ARGS");
  } else if (http_status == HTTP_BUSY) {
    strcpy(outputstr, "HTTP_BUSY");
  } else if (http_status == HTTP_BUFFER_OVERFLOW) {
    strcpy(outputstr, "HTTP_BUFFER_OVERFLOW");
  } else if (http_status == HTTP_NOT_ENOUGH_STORAGE) {
    strcpy(outputstr, "HTTP_NOT_ENOUGH_STORAGE");
  } else if (http_status == HTTP_INTERNAL_INCONSISTENCY) {
    strcpy(outputstr, "HTTP_INTERNAL_INCONSISTENCY");
  } else if (http_status == HTTP_INVALID_BRIDGE_RESPONSE) {
    strcpy(outputstr, "HTTP_INVALID_BRIDGE_RESPONSE");
  } else {
    strcpy(outputstr, "Unknown error?!");
  }
  text_layer_set_text(&textlayer, outputstr);
}

void http_cookie_get_callback(int32_t request_id, Tuple* result,
			      void* context) {
  if (request_id != HTTP_TZINFO_GET_REQ) {
    text_layer_set_text(&textlayer, "Unknown request ID");
    return;
  }
  if (result->key == HTTP_COOKIE_TZINFO) {
    strncpy(outputstr, (char *)result->value, 128);
    outputstr[127] = '\n';
    text_layer_set_text(&textlayer, outputstr);
  } else {
    text_layer_set_text(&textlayer, "Unknown key received");
  }
}

void handle_init(AppContextRef ctx) {

  window_init(&window, "Window Name");
  window_stack_push(&window, true /* Animated */);
  text_layer_init(&textlayer, window.layer.frame);
  text_layer_set_font(&textlayer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text(&textlayer, "Loading...");
  layer_add_child(&window.layer, &textlayer.layer);

  http_set_app_id(HTTP_APP_ID);
  http_register_callbacks((HTTPCallbacks) {
      .failure = http_cookie_failed_callback,
  	.cookie_get = http_cookie_get_callback,
  	}, ctx);
  http_cookie_get(0, 1);
  text_layer_set_text(&textlayer, "Request sent.");
}


void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .messaging_info = {
      .buffer_sizes = {
	.inbound = 124,
	.outbound = 256
      }
    }
  };
  app_event_loop(params, &handlers);
}
