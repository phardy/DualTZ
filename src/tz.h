#define TZ_NAME_LEN 15
#define TZ_OFFSET_LEN 6
typedef struct {
  char tz_name[TZ_NAME_LEN+1];
  int32_t remote_tz_offset;
  int32_t local_tz_offset;
} TZInfo;



void format_timezone(TZInfo *tz, char *str);
