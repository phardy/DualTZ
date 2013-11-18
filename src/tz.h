#define TZ_NAME_LEN 15
#define TZ_OFFSET_LEN 6
typedef struct {
  char tz_name[TZ_NAME_LEN+1];
  int32_t tz_offset;
} TZInfo;

// static TZInfo UTC = {"UTC", 0, 0, NULL};

void format_timezone(TZInfo *tz, char *str);
