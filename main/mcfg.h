#define NTP_SERVER          "pool.ntp.org"
#define TARGET_HOST         "example1.com"  // choose your primary target machine
#define SMART_TARGET_HOST   "example2.com"  // choose your secondary target machine
#define OTA_MACHINE         "example3.com"  // machine to fetch OTAs from via http
#define OTA_IMAGE           "ultra-remote-idf.bin"

#ifdef ESP_ARDUINO_VERSION          
#define TARGET_PORT         2345            // choose your primary target port
#define SMART_TARGET_PORT   2346            // choose your secondary target port
#define BUTTON              4               // choose button to trigger wakeup
#else
#define TARGET_PORT         "2345"          // choose your primary target port
#define SMART_TARGET_PORT   "2346"          // choose your secondary target port
#define BUTTON              2               // choose button to trigger wakeup
#endif

#define WIFI0_SSID      ""
#define WIFI0_PASSWORD  ""

#define WIFI1_SSID      "name_accesspoint1"
#define WIFI1_PASSWORD  "pass_accesspoint1"

#define WIFI2_SSID      "name_accesspoint2"
#define WIFI2_PASSWORD  "pass_accesspoint2"

#define WIFI3_SSID      "name_accesspoint3"
#define WIFI3_PASSWORD  "pass_accesspoint3"

#define WIFI4_SSID      "name_accesspoint4"
#define WIFI4_PASSWORD  "pass_accesspoint4"

#define WIFI5_SSID      "name_accesspoint5"
#define WIFI5_PASSWORD  "pass_accesspoint5"

