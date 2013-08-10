#include <string.h>

// Do not define if building watch apps that will talk to httpebble on iOS.
#define ANDROID

#define WATCHFACE_APP_UUID { 0x91, 0x41, 0xB6, 0x28, 0xBC, 0x89, 0x49, 0x8E, 0xB1, 0x47, 0xE2, 0x7C, 0x6C, 0xDB, 0x44, 0x73 }
#define SELECTOR_APP_UUID { 0x91, 0x41, 0xB6, 0x28, 0xBC, 0x89, 0x49, 0x8E, 0xB1, 0x47, 0xCC, 0xF3, 0x29, 0xEE, 0x1F, 0x83 }
// Use the same httpebble app ID everywhere to share the cookie dict.
#define HTTP_APP_ID 5887304

#define HTTP_TZINFO_GET_REQ 0
#define HTTP_TZINFO_SET_REQ 1
#define HTTP_COOKIE_TZINFO 1

#define TZ_NAME_LEN 15
#define TZ_OFFSET_LEN 6
typedef struct {
  char tz_name[TZ_NAME_LEN+1];
  char tz_offset[TZ_OFFSET_LEN+1];
  int32_t tz_seconds;
  bool tz_dst;
} TZInfo;

typedef enum {
  local,
  utc,
  remote
} TZState;

static TZInfo UTC = {"UTC", "+0", 0, NULL};
