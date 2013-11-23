#include <pebble.h>

void debug_log(char *err) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "DualTZ: storage write failed %s", err);
}

void debug_storage_write(int ret) {
  if (ret >= 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "DualTZ: storage write %i bytes", ret);
  } else {
    switch (ret) {
      case E_ERROR:
	debug_log("E_ERROR");
	break;
      case E_UNKNOWN:
	debug_log("E_UNKNOWN");
	break;
      case E_INVALID_ARGUMENT:
	debug_log("E_INVALID_ARGUMENT");
	break;
      case E_OUT_OF_MEMORY:
	debug_log("E_OUT_OF_MEMORY");
	break;
      case E_OUT_OF_STORAGE:
	debug_log("E_OUT_OF_STORAGE");
	break;
      case E_OUT_OF_RESOURCES:
	debug_log("E_OUT_OF_RESOURCES");
	break;
      case E_RANGE:
	debug_log("E_RANGE");
	break;
      case E_DOES_NOT_EXIST:
	debug_log("E_DOES_NOT_EXIST");
	break;
      case E_INVALID_OPERATION:
	debug_log("E_INVALID_OPERATION");
	break;
      case E_BUSY:
	debug_log("E_BUSY");
	break;
      default:
	APP_LOG(APP_LOG_LEVEL_DEBUG, "DualTZ: storage write failed %i", ret);
	break;
      }
  }
}
