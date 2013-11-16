// Functions and structures used by both apps

#define TZ_NAME_LEN 15
#define TZ_OFFSET_LEN 6
typedef struct {
  char tz_name[TZ_NAME_LEN+1];
  int8_t tz_hours;
  int8_t tz_minutes;
  bool tz_dst;
} TZInfo;

typedef enum {
  local,
  utc,
  remote
} TZState;

// static TZInfo UTC = {"UTC", 0, 0, NULL};

void format_timezone(TZInfo *tz, char *str);

void parse_timezone(char *str, TZInfo *tz);
