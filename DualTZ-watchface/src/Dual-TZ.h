void http_time_callback (int32_t utc_offset_seconds, bool is_dst,
			 uint32_t unixtime, const char* tz_name,
			 void* context);
void initLayerPathAndCenter (Layer *layer, GPath *path,
			     const GPathInfo *pathInfo,
			     const void *updateProc);
void hour_display_layer_update_callback (Layer *me, GContext* ctx);
void minute_display_layer_update_callback (Layer *me, GContext* ctx);
void update_digital_time (PblTm *time);
void handle_second_tick (AppContextRef ctx, PebbleTickEvent *t);
void display_init (AppContextRef *ctx);
void handle_init (AppContextRef ctx);
void handle_deinit (AppContextRef ctx);
void pbl_main (void *params);
