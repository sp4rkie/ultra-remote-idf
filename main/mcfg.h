#define TARGET_PORT

#define NTP_SERVER          _w1("pool.ntp.org")
#define STD_TARGET_HOST     _w1("example1.com")  // choose your primary target machine
#define SMART_TARGET_HOST   _w1("example2.com")  // choose your secondary target machine
#define DOOR_TARGET_HOST    _w1("example3.com")  // choose your tertiary target machine
#define OTA_MACHINE             "example4.com"   // machine to fetch OTAs from via http
#define OTA_IMAGE               "ultra-remote-idf.bin" 

#define DOOR_CMD_OPUL       _w1("aa")  
#define DOOR_CMD_OPL        _w1("ab")  
#define DOOR_CMD_OPLoff     _w1("ac") 

#ifdef ESP_ARDUINO_VERSION          
#define STD_TARGET_PORT     2345            // choose your primary target port
#define SMART_TARGET_PORT   2346            // choose your secondary target port
#define DOOR_TARGET_PORT    2347            // choose your tertiary target port
#else
#define STD_TARGET_PORT     _w1("2345")
#define SMART_TARGET_PORT   _w1("2346")
#define DOOR_TARGET_PORT    _w1("2347")
#endif

#define WIFI0_SSID          _w1("")
#define WIFI0_PASSWORD      _w1("")

#define WIFI1_SSID          _w1("name_accesspoint1")
#define WIFI1_PASSWORD      _w1("pass_accesspoint1")

#define WIFI2_SSID          _w1("name_accesspoint2")
#define WIFI2_PASSWORD      _w1("pass_accesspoint2")

#define WIFI3_SSID          _w1("name_accesspoint3")
#define WIFI3_PASSWORD      _w1("pass_accesspoint3")

#define WIFI4_SSID          _w1("name_accesspoint4")
#define WIFI4_PASSWORD      _w1("pass_accesspoint4")

#define WIFI5_SSID          _w1("name_accesspoint5")
#define WIFI5_PASSWORD      _w1("pass_accesspoint5")

