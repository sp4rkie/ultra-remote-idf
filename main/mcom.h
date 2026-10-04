/*
 *  extract contents with:
 *
 *      clear; egrep -e '---(vvv|\^\^\^)---|MCOM_ARD'  ~/esp/mcom.h
 *      clear; egrep -e '---(vvv|\^\^\^)---|MCOM_ARD_' ~/esp/mcom.h
 *
 * contents here mainly based on:
 * esp-idf.master/examples/protocols/static_ip/main/static_ip_example_main.c
 * esp-idf.master/examples/common_components/protocol_examples_common/wifi_connect.c
 *
 * many functions are located in:
 * esp-idf.master/components/esp_netif/lwip/esp_netif_lwip.c
 * esp-idf.master/components/esp_netif/include/esp_netif_types.h
 */


/*
 * MCOM_ARD - which half of this file a translation unit gets.
 *
 * CONFIG_ARDUINO_VARIANT alone only says arduino-esp32 is somewhere in the component graph: it is a
 * Kconfig string that exists the moment the component is linked, whatever the including file wants.
 * the arduino half includes "WiFi.h", a C++ header, so no .c file could ever compile it - adding
 * __cplusplus changes no existing build, and lets a .c file take the idf half in a project that still
 * links arduino libraries through a .cpp shim (ultra_temp: GxEPD2, DallasTemperature).
 */
#if defined(CONFIG_ARDUINO_VARIANT) && defined(__cplusplus)
#define MCOM_ARD
#endif
#if !defined(_MCOM_MINIMAL_)
/* ---vvv--- intro ---vvv-------------------------------------------------------------------------------------- */
// e.g. ultra-tester-2-0078
_i8 DEVICE_FW[32] = PROJECT "-" CDEF2STR(ENTITY) "-" SERNO;

#if defined(MCOM_ARD)

#define NO_DHCP
//#define DUMP_SOME                       // massive struct dump/ must have already started wifi for that/ ard-esp32: must be called after WiFi.begin()
#define ACCESSPOINT_CONNECT_BOOSTER     // required for host6 even with NO DHCP/ but should be avoided for all others to gain 10ms!!
                                        // UPDATE as of 2024_10_07 ACTIVATES AUTOMATICALLY FOR host10 ONLY
                                        // aka [ AUTO ACCESSPOINT_CONNECT_BOOSTER OFF ]
#else // defined(MCOM_ARD)

//---vvv-------------- WiFi / no ETH --------------------
#if !defined(ETH_OPMODE)
#define NO_DHCP                         // helps mostly for host15, voids WIFI_ALLOW_GRACE / not required (AKA contra productive) for ETH
#endif
//---^^^-------------- WiFi / no ETH --------------------
//#define WIFI_ALLOW_GRACE                // WARNING: WIFI_ALLOW_GRACE does not trigger if NO DHCP set/ BUT isn't required anyway if NO DHCP is set
                                          // WARNING II: once set grace remains active forever, no matter if SSID changes etc..
//#define WIFI_RESTORE                    // restore wifi_set_country_code
//#define DUMP_SOME                       // massive struct dump/ must have already started wifi for that/ ard-esp32: must be called after WiFi.begin()
#define DUMP_RSSI                       // some RSSI infos
/*
 * CACHE_CHANNEL - MEASURED AND REJECTED 2026_08_15 (entities 2 and 53). REDUNDANT.
 *
 * theory: ultra_ap runs esp32-57 on CHANNEL 11, the top of the 2.4GHz sweep, so without a hint
 * the STA would probe ~10 channels before finding the SSID, and w_link_up costs 24..31ms.
 *
 * WRONG - THE DRIVER ALREADY DOES THIS. because we run esp_wifi_set_storage(WIFI_STORAGE_FLASH)
 * with cfg.nvs_enable at its default 1, libnet80211 persists sta.chan and a 700 byte sta.apinfo
 * blob across boots. an nvs dump of entity 2 reads literally [ sta.chan: 11 ]. so the connect
 * scan was never sweeping, and w_link_up is auth + association + the WPA2 4-way handshake:
 *
 *      entity 2   26,277..36,035us  vs  25,972..26,821 baseline
 *      entity 53  24,092..33,646us  vs  24,148..25,867 baseline
 *
 * nothing gained on either. reverted - and note it is the SAME sta.* NVS cache that holds the PMK
 * (see ur_wifi_start()), which is now the third thing WIFI_STORAGE_FLASH quietly pays for.
 *
 * if you ever DO enable it, the two fixes below are already in place and are needed:
 *  - the fill lives in ur_handler_on_wifi_connect(), not in mysend() behind [ #if DEBUG > 1 ],
 *    where it never ran at DEBUG 1 and made this a silent no-op
 *  - mysend() clears the pin when association fails, otherwise bssid_set locks the remote to a
 *    single AP forever
 * one real argument for it that this bench cannot show: bssid_set also stops roaming, and the
 * ROTA2I note in the ultra_remote_mini test cases says a second micro accesspoint severely hurts.
 * against it: a remote is mobile, so a pin to a now distant AP costs a whole failed wakeup.
 */
//#define CACHE_CHANNEL                   // cache BSSID and channel

/*
 * LINK_SETTLE_US - the idf equivalent of the arduino ACCESSPOINT_CONNECT_BOOSTER (2026_08_15)
 *
 * the arduino side ladder in wait4wifi() is the clue (stat, arduino, TETHER_SSID):
 *      1ms -> 1101      2ms -> 162 sustained      3..5ms -> 164..167      10ms -> 169
 * i.e. sending IMMEDIATELY after the link reports up costs ~1s, and 2ms of patience removes it.
 *
 * TWO reasons that never carried over to idf:
 *  - arduino-esp32 runs configTICK_RATE_HZ 1000, we run FREERTOS_HZ 100. so the literal port,
 *    vTaskDelay(pdMS_TO_TICKS(2)), rounds to ZERO ticks and does nothing at all. hence a busy
 *    wait here - the wifi task (prio 23) preempts us anyway, so it does not starve the handshake
 *  - we are even more eager than arduino was: arduino waited for WL_CONNECTED (aka got-IP), but
 *    with NO_DHCP our static IP is applied from the STA_CONNECTED handler, which fires right
 *    after ASSOCIATION - possibly before the 4-way handshake finished and the AP installed keys.
 *    the first frame then goes into a hole and we wait out a retransmit timer
 *
 * MEASURED AND REJECTED against esp32-57 (entity 53, ~100 cycles per arm). the tail is counted as
 * the share of cycles at least 200ms above that arm's OWN median, so the added delay cannot skew
 * the comparison:
 *
 *      no settle       median 115   tail 6%   (n=102)
 *      2000            median 115   tail 7%   (n=112)
 *      100000          median 205   tail 6%   (n=119)   <- median moved, so the delay IS active
 *
 * 100ms of patience changes nothing. against THIS accesspoint the first frame is not going into a
 * hole, and "link not settled yet" is not the mechanism. left off.
 *
 * DO NOT read that as the arduino booster having been wrong. it was measured against TETHER_SSID,
 * a SMARTPHONE HOTSPOT - the case that actually hurts in the field, where runs of ~140ms carry a
 * lone ~1400ms outlier. that scenario has never been tried on idf, and this is the knob for it:
 * uncomment, point test_case[0] at TETHER_SSID and walk the ladder like the arduino side did
 */
//#define WIFI_DEAUTH_SETTLE_US 20000     // tried 2026_08_15: assoc failures 7% -> 8%, i.e. no effect
//#define LINK_SETTLE_US 200000
#include "esp_rom_sys.h"                    // esp_rom_delay_us()

/*
 * FORCE_2G4_BAND (experiment 2026_08_15) - dual band chips only, i.e. the C5 here
 *
 * nothing in this codebase ever calls esp_wifi_set_band_mode(), so a 5G capable chip sits in the
 * WIFI_BAND_MODE_AUTO default even while associated on a 2.4GHz channel. this was dismissed
 * earlier as an explanation for SCAN time - correctly, w_link_up is 24..26ms and fine - but it
 * has never been tested against the ~301ms downlink stalls that hit the C5 on a quarter to a
 * third of its wakeups and no other chip in the same 0.25m2 patch
 *
 * ATTENTION: do not enable this for an entity whose target is ROTA5G_SSID - that one IS 5GHz and
 * C5 only. a real implementation would derive the band from the accesspoint rather than force it
 */
//#define FORCE_2G4_BAND

/*
 * CACHE_TARGET_IP - keep the resolved target address in RTC memory across deep sleep, so
 * getaddrinfo() only runs when the target or the accesspoint changed
 *
 * measured 2026_08_15: entity 53 (C5) spends 304..446ms in getaddrinfo() on ~80% of its wakeups
 * and 4.9ms on the other 20%, entity 2 does the identical lookup in 4.4..5.4ms nearly every time.
 * a tcpdump on host4 (the DNS server, 192.168.0.24) shows ONE query per wakeup, answered in
 * ~400us, no retransmissions - so the 304ms is spent inside the C5, not on the network, and the
 * root cause is still unknown. this sidesteps it and pays for itself on the other chips too.
 *
 * NOT free: the DNS query had been warming the ARP cache for the gateway. without it the TCP SYN
 * pays for the ARP instead, so expect a few ms back in w_tcp_connect
 */
#define CACHE_TARGET_IP

/*
 * CACHE_GW_MAC - the other half of CACHE_TARGET_IP: keep the gateway's MAC in RTC memory and
 * prime a STATIC lwIP ARP entry the moment the static IP is applied, so the first packet after
 * association goes straight out instead of waiting a round trip for the ARP reply
 *
 * measured on entity 2: killing the DNS lookup moved 4.5ms into w_tcp_connect (2.8 -> 7.0ms),
 * i.e. the query had been doing the ARP for free. this is what recovers that
 *
 * ATTENTION: needs CONFIG_LWIP_DHCPS=y in sdkconfig.defaults. esp-idf has no separate knob for
 * ETHARP_SUPPORT_STATIC_ENTRIES, it is tied to the DHCP SERVER option, so that server rides along
 * in the image unused. there is an #error next to the code if you forget
 */
#define CACHE_GW_MAC
#if defined(CACHE_GW_MAC)
#include "lwip/etharp.h"

/*
 * ETHARP_SUPPORT_STATIC_ENTRIES is not a knob of its own in esp-idf - components/lwip/port/
 * include/lwipopts.h ties it to CONFIG_LWIP_DHCPS, which most of our sdkconfig.defaults set to n.
 * only sdkconfig.defaults_idf_{esp32,SUPERMINI_4MBFLASH_noPSRAM_noJTAG_noDEBUG,
 * C5DEVKIT_8MBFLASH_noDEBUG} were switched to y on 2026_08_15.
 *
 * mcom.h is shared by EVERY entity, so SELF-DISABLE on the others rather than break their builds.
 * do not turn this back into an #error: it takes out every entity whose SDK_VERS was not updated
 * (found the hard way - ultra_remote on esp32_16MBFLASH)
 */
#if !ETHARP_SUPPORT_STATIC_ENTRIES
#pragma message("CACHE_GW_MAC disabled here: needs CONFIG_LWIP_DHCPS=y in this target's sdkconfig.defaults")
#undef CACHE_GW_MAC
#endif
#endif  // CACHE_GW_MAC

/*
* the declarations this feature needs live under WIFI_INITIATOR||WIFI_TARGET and NO_DHCP further
* down, but the code using them sat under CACHE_GW_MAC alone - so an ETH_INITIATOR build compiled
* the uses and never got the declarations (found the hard way - ultra_espnow_gw on esp32-77).
* derive one symbol here and let the use sites test that instead.
*/
#if defined(CACHE_GW_MAC) && defined(NO_DHCP) && (defined(WIFI_INITIATOR) || defined(WIFI_TARGET))
#define CACHE_GW_MAC_ACTIVE
#endif


/*
 * EARLY_WIFI_PS - MEASURED AND REJECTED 2026_08_15, entity 53. DON'T RETRY.
 *
 * theory: ur_connect() calls esp_wifi_set_ps(type) only AFTER ur_wifi_connect(), i.e. after
 * esp_wifi_connect() kicked the association off. the driver default is WIFI_PS_MIN_MODEM, so the
 * association might be negotiated with the power management bit still set, the AP would believe
 * the STA sleeps between DTIMs, and a downlink DNS reply would wait for a beacon. that would have
 * explained the C5 spending 304..446ms in getaddrinfo() on ~80% of its wakeups while doing the
 * same lookup in 4.9ms on the other 20%.
 *
 * threading the ps type down to ur_wifi_start() and applying it right after esp_wifi_start(),
 * i.e. well before esp_wifi_connect() (verified on the wire: the mark fired at +300us), changed
 * NOTHING: 8 of 10 cycles still 304,268..348,444us, 2 of 10 still ~3,700us. identical bimodal
 * split, identical ratio. power save is not the mechanism.
 *
 * also ruled out: it is not an lwIP DNS retransmission - DNS_TMR_INTERVAL is 1000ms and is not
 * overridden anywhere, so a retry cannot land on a 304ms floor.
 */
//#define EARLY_WIFI_PS

#define ESP_ARDUINO_VERSION 100         // make it always true to use associated code on ESP
#define ESP_ARDUINO_VERSION_VAL(a, b, c) 1

#endif // defined(MCOM_ARD)

// some i.... did remove this on some archs?!
#if !defined(MACSTR)
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"
#endif
#if !defined(MAC2STR)
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#endif
/* ---^^^--- intro ---^^^-------------------------------------------------------------------------------------- */


/* ---vvv--- debug ---vvv-------------------------------------------------------------------------------------- */

#if defined(MCOM_ARD)

#include "WiFi.h"       // required by WiFiEvent()
#include "WiFiType.h"   // needed for WL_NO_SHIELD...
#include "esp_wifi.h"   // required by esp_wifi_stop(), esp_wifi_connect() / must precede "mcom.h"
#include "esp_now.h"
#include "rom/rtc.h"    // required by rtc_get_reset_reason()


// USE THIS!@@@@@@@
// uint64_t fr_start = esp_timer_get_time();
//

#if ESP_ARDUINO_VERSION > ESP_ARDUINO_VERSION_VAL(2,0,17)       // aka since 3.0.4
#define tstamp() esp_log_timestamp()
#else
#define tstamp() (__u32)esp_log_timestamp()     // is natively: type 'uint32_t' {aka 'unsigned int'}
#endif

#else   // defined(MCOM_ARD)

#include "esp_sleep.h"          // required by touch_pad_t 
#include "esp_timer.h"          // required by esp_timer_get_time()

//
// for "esp_wifi.h" to compile on P4 you must have at least:
// 
// dependencies:
//   espressif/esp_wifi_remote: '*'
// 
// in your idf_component.yml file
//
#include "esp_wifi.h"           // required by give_wifi_event() etc...
#include "esp_now.h"
#include "esp_log.h"            // required by esp_log_timestamp() 
#include "esp_mac.h"            // for setting esp_base_mac_addr_set() in init_2nd()
#include "rom/rtc.h"            // required for rtc_get_reset_reason 
#include "string.h"             // required by ffs() 
#include "driver/gpio.h"        // required by gpio_config_t
#include "driver/rtc_io.h"      // required by rtc_gpio_pullup_dis/en()

#define tstamp() esp_log_timestamp()

#endif  // defined(MCOM_ARD)

/*
 * profiling hook for the WiFi wakeup -> send path
 *
 * the ESPNOW path could be measured by inlining ESPNOW_WIFI_SETUP() into the test bed, leaving
 * mcom.h untouched. that does not work for WiFi: the interesting time sits inside ur_wifi_start(),
 * ur_wifi_sta_do_connect() and mysend(), which are real functions and far too big to inline
 *
 * so the marks live here, but expand to nothing unless the app defines WTPROF BEFORE including
 * mcom.h -> productive firmware pays exactly nothing, same deal as DEBUG_TPROF
 *
 * in ultra_remote_mini.c:
 *      #define WTPROF TPROF        // (TPROF block must be moved ahead of #include "mcom.h")
 */
#if !defined(WTPROF)
#define WTPROF(nam) do { } while (0)
#endif

//
// uint64_t        esp_sleep_get_gpio_wakeup_status(void);  // only esp32c3 defines this?!?
// uint64_t        esp_sleep_get_ext1_wakeup_status(void);
// touch_pad_t     esp_sleep_get_touchpad_wakeup_status(void);
// esp_sleep_wakeup_cause_t 
//                 esp_sleep_get_wakeup_cause(void);
//
void 
print_wakeup_touchpad()
{
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C5)
    PR00("print_wakeup_touchpad() not supported\n"); 
#else
#if defined(MCOM_ARD)
    _i32 touchPin = esp_sleep_get_touchpad_wakeup_status();
#else
    touch_pad_t touchPin = esp_sleep_get_touchpad_wakeup_status();
#endif

    switch (touchPin) {
        case 0  : PR00("Touch detected on GPIO 4\n"); break;
        case 1  : PR00("Touch detected on GPIO 0\n"); break;
        case 2  : PR00("Touch detected on GPIO 2\n"); break;
        case 3  : PR00("Touch detected on GPIO 15\n"); break;
        case 4  : PR00("Touch detected on GPIO 13\n"); break;
        case 5  : PR00("Touch detected on GPIO 12\n"); break;
        case 6  : PR00("Touch detected on GPIO 14\n"); break;
        case 7  : PR00("Touch detected on GPIO 27\n"); break;
        case 8  : PR00("Touch detected on GPIO 33\n"); break;
        case 9  : PR00("Touch detected on GPIO 32\n"); break;
        default : PR00("wakeup not by touchpad\n"); break;
    }
#endif
}

void
print_wakeup_gpio_wakeup()
{
#if defined(SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP)    // only esp32c3 defines this?!?
    _u64 GpioPin = esp_sleep_get_gpio_wakeup_status();

    PR00("wakeup by gpio [ 0x%016llx ], PIN: %u\n", GpioPin, ffs(GpioPin) - 1);
#else

    /*
     * there solely exists exactly the one defined by 
     *     esp_sleep_enable_ext0_wakeup(PIN, 0);
     */
    PR00("wakeup by ext0 PIN X, check esp_sleep_enable_ext0_wakeup()\n");
#endif
}

void 
print_wakeup_ext1_wakeup()
{

#if defined(CONFIG_IDF_TARGET_ESP32C3)
// WAS JETZT???????????

    PR00("print_wakeup_ext1_wakeup() not exist on C3\n");

#else

    _u64 GpioPin = esp_sleep_get_ext1_wakeup_status();

#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C5)
    // is long unsigned int for these archs
    PR00("wakeup by ext1 [ 0x%016llx ], PIN: %u\n", GpioPin, ffs(GpioPin) - 1);
#else
    // is long long unsigned int for these archs
    PR00("wakeup by ext1 [ 0x%016llx ], PIN: %u\n", GpioPin, ffs(GpioPin) - 1);
#endif


#endif

}

void 
print_wakeup_reason()
{
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            PR00("wakeup caused by external signal using RTC_IO (EXT0)\n"); 
            print_wakeup_gpio_wakeup();
            break;
        case ESP_SLEEP_WAKEUP_EXT1: 
            PR00("wakeup caused by external signal using RTC_CNTL (EXT1)\n"); 
            print_wakeup_ext1_wakeup();
            break;
        case ESP_SLEEP_WAKEUP_TIMER: 
            PR00("wakeup caused by timer\n"); 
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: 
            PR00("wakeup caused by touchpad\n"); 
            print_wakeup_touchpad();
            break;
        case ESP_SLEEP_WAKEUP_ULP: 
            PR00("wakeup caused by ULP program\n"); 
            break;
        default: 
            PR00("wakeup was not caused by deep sleep (reason == %u)\n", wakeup_reason); 
            break;
    }
}

void 
print_reset_reason(_u32 cpu)
{
    _i32 reason = rtc_get_reset_reason(cpu);

    PR00("CPU%d reset reason: ", cpu);
    switch (reason) {
        case NO_MEAN:               PR00("NO_MEAN\n"); break;
        case POWERON_RESET:         PR00("Vbat power on reset\n"); break;
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32S3)
        case TG1WDT_CPU_RESET:      PR00("Time Group1 reset CPU\n"); break;
        case SUPER_WDT_RESET:       PR00("super watchdog reset digital core and rtc module\n"); break;
        case GLITCH_RTC_RESET:      PR00("glitch reset digital core and rtc module\n"); break;
        case EFUSE_RESET:           PR00("efuse reset digital core\n"); break;
        case USB_UART_CHIP_RESET:   PR00("usb uart reset digital core\n"); break;
        case USB_JTAG_CHIP_RESET:   PR00("usb jtag reset digital core\n"); break;
        case POWER_GLITCH_RESET:    PR00("power glitch reset digital core and rtc module\n"); break;
#elif defined(CONFIG_IDF_TARGET_ESP32C5)

// put something here

#elif defined(CONFIG_IDF_TARGET_ESP32P4)

// put something here

#else
        case SW_RESET:              PR00("Software reset digital core\n"); break;
        case OWDT_RESET:            PR00("Legacy watch dog reset digital core\n"); break;
        case SDIO_RESET:            PR00("Reset by SLC module, reset digital core\n"); break;
        case TGWDT_CPU_RESET:       PR00("Time Group reset CPU\n"); break;
        case SW_CPU_RESET:          PR00("Software reset CPU\n"); break;
        case EXT_CPU_RESET:         PR00("for APP CPU, reseted by PRO CPU\n"); break;
#endif
#if !defined(CONFIG_IDF_TARGET_ESP32C5) && !defined(CONFIG_IDF_TARGET_ESP32P4)
        case DEEPSLEEP_RESET:       PR00("Deep Sleep reset digital core\n"); break;
        case TG0WDT_SYS_RESET:      PR00("Timer Group0 Watch dog reset digital core\n"); break;
        case TG1WDT_SYS_RESET:      PR00("Timer Group1 Watch dog reset digital core\n"); break;
        case RTCWDT_SYS_RESET:      PR00("RTC Watch dog Reset digital core\n"); break;
        case INTRUSION_RESET:       PR00("Instrusion tested to reset CPU\n"); break;
        case RTCWDT_CPU_RESET:      PR00("RTC Watch dog Reset CPU\n"); break;
        case RTCWDT_BROWN_OUT_RESET: PR00("Reset when the vdd voltage is not stable\n"); break;
        case RTCWDT_RTC_RESET:      PR00("RTC Watch dog reset digital core and rtc module\n"); break;
#endif
        default:                    PR00("unknown reset reason: %d\n", reason); break;
    }
}

#if defined(MCOM_ARD)

const _i8 *
give_wifi_status(_u8 stat)
{
  switch (stat) {
    case WL_NO_SHIELD:                      return "WL_NO_SHIELD";
#if ESP_ARDUINO_VERSION > ESP_ARDUINO_VERSION_VAL(2,0,17)       // aka since 3.0.4
    case WL_STOPPED:                        return "WL_STOPPED";
#endif
    case WL_IDLE_STATUS:                    return "WL_IDLE_STATUS";
    case WL_NO_SSID_AVAIL:                  return "WL_NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:                 return "WL_SCAN_COMPLETED";
    case WL_CONNECTED:                      return "WL_CONNECTED";
    case WL_CONNECT_FAILED:                 return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST:                return "WL_CONNECTION_LOST";
    case WL_DISCONNECTED:                   return "WL_DISCONNECTED";
    default:                                return "unknown";
  }
}

#define WIFI_DIAG(a) \
{ \
    _u8 macAddr[6]; \
\
    PR00("---v--- WIFI diag ---v---\n"); \
    PR00("WiFi status: %s\n", give_wifi_status(WiFi.status())); \
    WiFi.macAddress(macAddr); \
    PR00("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], \
                                                 macAddr[1], \
                                                 macAddr[2], \
                                                 macAddr[3], \
                                                 macAddr[4], \
                                                 macAddr[5]); \
\
    /* TEST DIS: */ \
    /* NOT COMPILE??? PR00("MAC: " MACSTR "\n", MAC2STR(macAddr)); */ \
\
    if (a) { \
        PR00("MY IP:  %s\n", WiFi.localIP().toString().c_str()); \
        PR00("GW IP:  %s\n", WiFi.gatewayIP().toString().c_str()); \
        PR00("subnet: %s\n", WiFi.subnetMask().toString().c_str()); \
        PR00("DNS0:   %s\n", WiFi.dnsIP(0).toString().c_str()); \
        PR00("DNS1:   %s\n", WiFi.dnsIP(1).toString().c_str()); \
        PR00("BSSID:  %s\n", WiFi.BSSIDstr().c_str()); \
        PR00("RSSI:   %d\n", WiFi.RSSI()); \
    } \
    PR00("---^--- WIFI diag ---^---\n"); \
}

// derived from components/arduino-esp32.idf-release_v5.3/libraries/WiFi/examples/WiFiClientEvents/WiFiClientEvents.ino
const _i8 *
give_wifi_event(_u8 event) 
{
//TP05
  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:               return "WiFi interface ready"; 
    case ARDUINO_EVENT_WIFI_SCAN_DONE:           return "Completed scan for access points"; 
    case ARDUINO_EVENT_WIFI_STA_START:           return "WiFi client started"; 
    case ARDUINO_EVENT_WIFI_STA_STOP:            return "WiFi clients stopped"; 
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:       return "Connected to access point"; 
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:    return "Disconnected from WiFi access point"; 
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE: return "Authentication mode of access point has changed"; 
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:          return "WIFI STA GOT IP";
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:         return "Lost IP address and IP address is reset to 0"; 
    case ARDUINO_EVENT_WPS_ER_SUCCESS:           return "WiFi Protected Setup (WPS: succeeded in enrollee mode"; 
    case ARDUINO_EVENT_WPS_ER_FAILED:            return "WiFi Protected Setup (WPS: failed in enrollee mode"; 
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:           return "WiFi Protected Setup (WPS: timeout in enrollee mode"; 
    case ARDUINO_EVENT_WPS_ER_PIN:               return "WiFi Protected Setup (WPS: pin code in enrollee mode"; 
    case ARDUINO_EVENT_WIFI_AP_START:            return "WiFi access point started"; 
    case ARDUINO_EVENT_WIFI_AP_STOP:             return "WiFi access point  stopped"; 
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:     return "Client connected"; 
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:  return "Client disconnected"; 
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:    return "Assigned IP address to client"; 
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:   return "Received probe request"; 
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:          return "AP IPv6 is preferred"; 
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:         return "STA IPv6 is preferred"; 
    case ARDUINO_EVENT_ETH_GOT_IP6:              return "Ethernet IPv6 is preferred"; 
    case ARDUINO_EVENT_ETH_START:                return "Ethernet started"; 
    case ARDUINO_EVENT_ETH_STOP:                 return "Ethernet stopped"; 
    case ARDUINO_EVENT_ETH_CONNECTED:            return "Ethernet connected"; 
    case ARDUINO_EVENT_ETH_DISCONNECTED:         return "Ethernet disconnected"; 
    case ARDUINO_EVENT_ETH_GOT_IP:               return "Obtained IP address"; 
    default:                                     return "NOT SPECIFIED";
  }
}

#else   // defined(MCOM_ARD)

#define WIFI_DIAG(a) 

const _i8 *
give_wifi_event(_u8 event) 
{
//TP05
  switch (event) {
    case WIFI_EVENT_WIFI_READY:             return "WiFi interface ready";
    case WIFI_EVENT_SCAN_DONE:              return "Completed scan for access points";
    case WIFI_EVENT_STA_START:              return "WiFi client started";
    case WIFI_EVENT_STA_STOP:               return "WiFi clients stopped";
    case WIFI_EVENT_STA_CONNECTED:          return "Connected to access point";
    case WIFI_EVENT_STA_DISCONNECTED:       WIFI_DIAG(0); return "Disconnected from WiFi access point";
    case WIFI_EVENT_STA_AUTHMODE_CHANGE:    return "Authentication mode of access point has changed";
    case WIFI_EVENT_STA_BSS_RSSI_LOW:       return "WiFi SYSTEM_EVENT_STA_BSS_RSSI_LOW";
    case WIFI_EVENT_STA_WPS_ER_SUCCESS:     return "WiFi Protected Setup (WPS): succeeded in enrollee mode";
    case WIFI_EVENT_STA_WPS_ER_FAILED:      return "WiFi Protected Setup (WPS): failed in enrollee mode";
    case WIFI_EVENT_STA_WPS_ER_TIMEOUT:     return "WiFi Protected Setup (WPS): timeout in enrollee mode";
    case WIFI_EVENT_STA_WPS_ER_PIN:         return "WiFi Protected Setup (WPS): pin code in enrollee mode";
    case WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP: return "WiFi SYSTEM_EVENT_STA_WPS_ER_PBC_OVERLAP";
    case WIFI_EVENT_AP_START:               return "WiFi access point started";
    case WIFI_EVENT_AP_STOP:                return "WiFi access point stopped";
    case WIFI_EVENT_AP_STACONNECTED:        return "Client connected";
    case WIFI_EVENT_AP_STADISCONNECTED:     return "Client disconnected";
    case WIFI_EVENT_AP_PROBEREQRECVED:      return "Received probe request";
    case WIFI_EVENT_ACTION_TX_STATUS:       return "WiFi SYSTEM_EVENT_ACTION_TX_STATUS";
    case WIFI_EVENT_ROC_DONE:               return "WiFi SYSTEM_EVENT_ROC_DONE";
    case WIFI_EVENT_STA_BEACON_TIMEOUT:     return "WiFi SYSTEM_EVENT_STA_BEACON_TIMEOUT";
    case WIFI_EVENT_FTM_REPORT:             return "WiFi SYSTEM_EVENT_FTM_REPORT";
#if ESP_ARDUINO_VERSION > ESP_ARDUINO_VERSION_VAL(2,0,17)       // aka since 3.0.4
    case WIFI_EVENT_CONNECTIONLESS_MODULE_WAKE_INTERVAL_START:  return "Connectionless module wake interval start";
    case WIFI_EVENT_AP_WPS_RG_SUCCESS:      return "Soft-AP wps succeeds in registrar mode";
    case WIFI_EVENT_AP_WPS_RG_FAILED:       return "Soft-AP wps fails in registrar mode";
    case WIFI_EVENT_AP_WPS_RG_TIMEOUT:      return "Soft-AP wps timeout in registrar mode";
    case WIFI_EVENT_AP_WPS_RG_PIN:          return "Soft-AP wps pin code in registrar mode";
    case WIFI_EVENT_AP_WPS_RG_PBC_OVERLAP:  return "Soft-AP wps overlap in registrar mode";
    case WIFI_EVENT_ITWT_SETUP:             return "iTWT setup";
    case WIFI_EVENT_ITWT_TEARDOWN:          return "iTWT teardown";
    case WIFI_EVENT_ITWT_PROBE:             return "iTWT probe";
    case WIFI_EVENT_ITWT_SUSPEND:           return "iTWT suspend";
    case WIFI_EVENT_NAN_STARTED:            return "NAN Discovery has started";
    case WIFI_EVENT_NAN_STOPPED:            return "NAN Discovery has stopped";
    case WIFI_EVENT_NAN_SVC_MATCH:          return "NAN Service Discovery match found";
    case WIFI_EVENT_NAN_REPLIED:            return "Replied to a NAN peer with Service Discovery match";
    case WIFI_EVENT_NAN_RECEIVE:            return "Received a Follow-up message";
    case WIFI_EVENT_NDP_INDICATION:         return "Received NDP Request from a NAN Peer";
    case WIFI_EVENT_NDP_CONFIRM:            return "NDP Confirm Indication";
    case WIFI_EVENT_NDP_TERMINATED:         return "NAN Datapath terminated indication";
    case WIFI_EVENT_TWT_WAKEUP:             return "TWT wakeup";
#if ESP_ARDUINO_VERSION > ESP_ARDUINO_VERSION_VAL(3,0,4)
    case WIFI_EVENT_HOME_CHANNEL_CHANGE:    return "WiFi home channel change，doesn't occur when scanning";
    case WIFI_EVENT_STA_NEIGHBOR_REP:       return "Received Neighbor Report response";
#endif
#endif
    case WIFI_EVENT_MAX:                    return "Invalid WiFi event ID";
    default:                                return "NOT SPECIFIED";
  }
}

// because of C5:  
//      esp_wifi_get_bandwidth -> esp_wifi_get_bandwidths
#if DEBUG > 1
#define WIFI_FIXER_DEBUG(str, mode) \
do { \
_i8 country_code[10]; \
wifi_ps_type_t ps_type; \
wifi_bandwidths_t bw; \
\
PR00(str); \
ESP_ERROR_CHECK(esp_wifi_get_ps(&ps_type)); \
ESP_ERROR_CHECK(esp_wifi_get_country_code(country_code)); \
ESP_ERROR_CHECK(esp_wifi_get_bandwidths(mode == WIFI_MODE_STA ? WIFI_IF_STA : WIFI_IF_AP, &bw)); \
PR00("get power save: %s country [%c%c%c] bw2G: %s bw5G: %s\n",  \
     give_ps(ps_type), \
     country_code[0], country_code[1], country_code[2], \
     give_bw(bw.ghz_2g), give_bw(bw.ghz_5g)); \
} while (0)
#else   // if DEBUG > 1
#define WIFI_FIXER_DEBUG(str, mode)
#endif  // if DEBUG > 1

#endif  // defined(MCOM_ARD)

const _i8 *
give_ps(_u32 ps)
{
//TP05
  switch (ps) {
    case WIFI_PS_NONE:                    return "WIFI_PS_NONE";
    case WIFI_PS_MIN_MODEM:               return "WIFI_PS_MIN_MODEM";
    case WIFI_PS_MAX_MODEM:               return "WIFI_PS_MAX_MODEM";
    default:                              return "unknown";
  }
}

const _i8 *
give_bw(_u32 bw)
{
//TP05
  switch (bw) {
    case WIFI_BW_HT20:                    return "WIFI_BW_HT20";
//    case WIFI_BW20:                       return "WIFI_BW20";
    case WIFI_BW_HT40:                    return "WIFI_BW_HT40";
//    case WIFI_BW40:                       return "WIFI_BW40";
    case WIFI_BW80:                       return "WIFI_BW80";
    case WIFI_BW160:                      return "WIFI_BW160";
    case WIFI_BW80_BW80:                  return "WIFI_BW80_BW80";
    default:                              return "unknown";
  }
}

const _i8 *
give_mode(_u32 mode)
{
//TP05
  switch (mode) {
    case WIFI_MODE_NULL:                  return "WIFI_MODE_NULL";
    case WIFI_MODE_STA:                   return "WIFI_MODE_STA";
    case WIFI_MODE_AP:                    return "WIFI_MODE_AP";
    case WIFI_MODE_APSTA:                 return "WIFI_MODE_APSTA";
    case WIFI_MODE_NAN:                   return "WIFI_MODE_NAN";
    case WIFI_MODE_MAX:                   return "WIFI_MODE_MAX";
    default:                              return "unknown";
  }
}

const _i8 *
give_wifi_err(_u32 err)
{
//TP05
  switch (err) {
    case ESP_ERR_WIFI_NOT_INIT:           return "WiFi driver was not installed by esp_wifi_init";
    case ESP_ERR_WIFI_NOT_STARTED:        return "WiFi driver was not started by esp_wifi_start";
    case ESP_ERR_WIFI_NOT_STOPPED:        return "WiFi driver was not stopped by esp_wifi_stop";
    case ESP_ERR_WIFI_IF:                 return "WiFi interface error";
    case ESP_ERR_WIFI_MODE:               return "WiFi mode error";
    case ESP_ERR_WIFI_STATE:              return "WiFi internal state error";
    case ESP_ERR_WIFI_CONN:               return "WiFi internal control block of station or soft-AP error";
    case ESP_ERR_WIFI_NVS:                return "WiFi internal NVS module error";
    case ESP_ERR_WIFI_MAC:                return "MAC address is invalid";
    case ESP_ERR_WIFI_SSID:               return "SSID is invalid";
    case ESP_ERR_WIFI_PASSWORD:           return "Password is invalid";
    case ESP_ERR_WIFI_TIMEOUT:            return "Timeout error";
    case ESP_ERR_WIFI_WAKE_FAIL:          return "WiFi is in sleep state(RF closed) and wakeup fail";
    case ESP_ERR_WIFI_WOULD_BLOCK:        return "The caller would block";
    case ESP_ERR_WIFI_NOT_CONNECT:        return "Station still in disconnect status";
    case ESP_ERR_WIFI_POST:               return "Failed to post the event to WiFi task";
    case ESP_ERR_WIFI_INIT_STATE:         return "Invalid WiFi state when init/deinit is called";
    case ESP_ERR_WIFI_STOP_STATE:         return "Returned when WiFi is stopping";
    case ESP_ERR_WIFI_NOT_ASSOC:          return "The WiFi connection is not associated";
    case ESP_ERR_WIFI_TX_DISALLOW:        return "The WiFi TX is disallowed";
    case ESP_ERR_WIFI_TWT_FULL:           return "no available flow id";
    case ESP_ERR_WIFI_TWT_SETUP_TIMEOUT:  return "Timeout of receiving twt setup response frame, timeout times can be set during twt setup";
    case ESP_ERR_WIFI_TWT_SETUP_TXFAIL:   return "TWT setup frame tx failed";
    case ESP_ERR_WIFI_TWT_SETUP_REJECT:   return "The twt setup request was rejected by the AP";
    case ESP_ERR_WIFI_DISCARD:            return "Discard frame";
    case ESP_ERR_WIFI_ROC_IN_PROGRESS:    return "ROC op is in progress";
    default:                              return "unknown";
  }
}

const _i8 *
give_disc_reason(_u8 reason) 
{ 
//TP05
  switch (reason) {
    case WIFI_REASON_UNSPECIFIED:                        return "UNSPECIFIED";
    case WIFI_REASON_AUTH_EXPIRE:                        return "AUTH_EXPIRE";
    case WIFI_REASON_AUTH_LEAVE:                         return "AUTH_LEAVE";
    case WIFI_REASON_ASSOC_EXPIRE:                       return "ASSOC_EXPIRE";
    case WIFI_REASON_ASSOC_TOOMANY:                      return "ASSOC_TOOMANY";
    case WIFI_REASON_NOT_AUTHED:                         return "NOT_AUTHED";
    case WIFI_REASON_NOT_ASSOCED:                        return "NOT_ASSOCED";
    case WIFI_REASON_ASSOC_LEAVE:                        return "ASSOC_LEAVE";
    case WIFI_REASON_ASSOC_NOT_AUTHED:                   return "ASSOC_NOT_AUTHED";
    case WIFI_REASON_DISASSOC_PWRCAP_BAD:                return "DISASSOC_PWRCAP_BAD";
    case WIFI_REASON_DISASSOC_SUPCHAN_BAD:               return "DISASSOC_SUPCHAN_BAD";
    case WIFI_REASON_BSS_TRANSITION_DISASSOC:            return "BSS_TRANSITION_DISASSOC";
    case WIFI_REASON_IE_INVALID:                         return "IE_INVALID";
    case WIFI_REASON_MIC_FAILURE:                        return "MIC_FAILURE";
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:             return "4WAY_HANDSHAKE_TIMEOUT";
    case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:           return "GROUP_KEY_UPDATE_TIMEOUT";
    case WIFI_REASON_IE_IN_4WAY_DIFFERS:                 return "IE_IN_4WAY_DIFFERS";
    case WIFI_REASON_GROUP_CIPHER_INVALID:               return "GROUP_CIPHER_INVALID";
    case WIFI_REASON_PAIRWISE_CIPHER_INVALID:            return "PAIRWISE_CIPHER_INVALID";
    case WIFI_REASON_AKMP_INVALID:                       return "AKMP_INVALID";
    case WIFI_REASON_UNSUPP_RSN_IE_VERSION:              return "UNSUPP_RSN_IE_VERSION";
    case WIFI_REASON_INVALID_RSN_IE_CAP:                 return "INVALID_RSN_IE_CAP";
    case WIFI_REASON_802_1X_AUTH_FAILED:                 return "802_1X_AUTH_FAILED";
    case WIFI_REASON_CIPHER_SUITE_REJECTED:              return "CIPHER_SUITE_REJECTED";
    case WIFI_REASON_TDLS_PEER_UNREACHABLE:              return "TDLS_PEER_UNREACHABLE";
    case WIFI_REASON_TDLS_UNSPECIFIED:                   return "TDLS_UNSPECIFIED";
    case WIFI_REASON_SSP_REQUESTED_DISASSOC:             return "SSP_REQUESTED_DISASSOC";
    case WIFI_REASON_NO_SSP_ROAMING_AGREEMENT:           return "NO_SSP_ROAMING_AGREEMENT";
    case WIFI_REASON_BAD_CIPHER_OR_AKM:                  return "BAD_CIPHER_OR_AKM";
    case WIFI_REASON_NOT_AUTHORIZED_THIS_LOCATION:       return "NOT_AUTHORIZED_THIS_LOCATION";
    case WIFI_REASON_SERVICE_CHANGE_PERCLUDES_TS:        return "SERVICE_CHANGE_PERCLUDES_TS";
    case WIFI_REASON_UNSPECIFIED_QOS:                    return "UNSPECIFIED_QOS";
    case WIFI_REASON_NOT_ENOUGH_BANDWIDTH:               return "NOT_ENOUGH_BANDWIDTH";
    case WIFI_REASON_MISSING_ACKS:                       return "MISSING_ACKS";
    case WIFI_REASON_EXCEEDED_TXOP:                      return "EXCEEDED_TXOP";
    case WIFI_REASON_STA_LEAVING:                        return "STA_LEAVING";
    case WIFI_REASON_END_BA:                             return "END_BA";
    case WIFI_REASON_UNKNOWN_BA:                         return "UNKNOWN_BA";
    case WIFI_REASON_TIMEOUT:                            return "TIMEOUT";
    case WIFI_REASON_PEER_INITIATED:                     return "PEER_INITIATED";
    case WIFI_REASON_AP_INITIATED:                       return "AP_INITIATED";
    case WIFI_REASON_INVALID_FT_ACTION_FRAME_COUNT:      return "INVALID_FT_ACTION_FRAME_COUNT";
    case WIFI_REASON_INVALID_PMKID:                      return "INVALID_PMKID";
    case WIFI_REASON_INVALID_MDE:                        return "INVALID_MDE";
    case WIFI_REASON_INVALID_FTE:                        return "INVALID_FTE";
    case WIFI_REASON_TRANSMISSION_LINK_ESTABLISH_FAILED: return "TRANSMISSION_LINK_ESTABLISH_FAILED";
    case WIFI_REASON_ALTERATIVE_CHANNEL_OCCUPIED:        return "ALTERATIVE_CHANNEL_OCCUPIED";
    case WIFI_REASON_BEACON_TIMEOUT:                     return "BEACON_TIMEOUT";
    case WIFI_REASON_NO_AP_FOUND:                        return "NO_AP_FOUND";
    case WIFI_REASON_AUTH_FAIL:                          return "AUTH_FAIL";
    case WIFI_REASON_ASSOC_FAIL:                         return "ASSOC_FAIL";
    case WIFI_REASON_HANDSHAKE_TIMEOUT:                  return "HANDSHAKE_TIMEOUT";
    case WIFI_REASON_CONNECTION_FAIL:                    return "CONNECTION_FAIL";
    case WIFI_REASON_AP_TSF_RESET:                       return "AP_TSF_RESET";
    case WIFI_REASON_ROAMING:                            return "ROAMING";
#if ESP_ARDUINO_VERSION > ESP_ARDUINO_VERSION_VAL(2,0,7)       
    case WIFI_REASON_ASSOC_COMEBACK_TIME_TOO_LONG:       return "ASSOC_COMEBACK_TIME_TOO_LONG";
#endif
    case WIFI_REASON_SA_QUERY_TIMEOUT:                   return "WIFI_REASON_SA_QUERY_TIMEOUT";
    case WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY:  return "WIFI_REASON_NO_AP_FOUND_W_COMPATIBLE_SECURITY";
    case WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD:  return "WIFI_REASON_NO_AP_FOUND_IN_AUTHMODE_THRESHOLD";
    case WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD:      return "WIFI_REASON_NO_AP_FOUND_IN_RSSI_THRESHOLD";
    default:                                             return "NOT SPECIFIED";
  }
}
/* ---^^^--- debug ---^^^-------------------------------------------------------------------------------------- */
#endif  // !defined(_MCOM_MINIMAL_)


/* ---vvv--- GPIO ---vvv--------------------------------------------------------------------------------------- */

#include "hal/gpio_ll.h"    // required for GPIO.in, GPIO.in1.val...

#define GPIO_ON_PIN(pin)    (pin) > 31 ? (GPIO.out1_w1ts.val = 1 << (pin) - 32) : (GPIO.out_w1ts = 1 << (pin))
#define GPIO_OFF_PIN(pin)   (pin) > 31 ? (GPIO.out1_w1tc.val = 1 << (pin) - 32) : (GPIO.out_w1tc = 1 << (pin))

#define ACT_HGH 0
#define ACT_LOW 1

#define ledctl(pin, on, act) ((on) ^ (act) ? GPIO_ON_PIN(pin) : GPIO_OFF_PIN(pin))

/* ---^^^--- GPIO ---^^^--------------------------------------------------------------------------------------- */


#if !defined(_MCOM_MINIMAL_)
/* ---vvv--- NVS ---vvv---------------------------------------------------------------------------------------- */

#include "nvs_flash.h"          // required for nvs_flash_init() in mcom.h

#define GET_NVS(var) \
do { \
    nvs_handle_t hndl; \
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &hndl)); \
    nvs_get_i32(hndl, CDEF2STR(var), &var); \
    nvs_close(hndl); \
} while (0)

#define SET_NVS(var, val) \
do { \
    nvs_handle_t hndl; \
    var = val; \
    ESP_ERROR_CHECK(nvs_open("storage", NVS_READWRITE, &hndl)); \
    ESP_ERROR_CHECK(nvs_set_i32(hndl, CDEF2STR(var), var)); \
    ESP_ERROR_CHECK(nvs_commit(hndl)); \
    nvs_close(hndl); \
} while (0)
/* ---^^^--- NVS ---^^^---------------------------------------------------------------------------------------- */


/* ---vvv--- monitor battery voltage ---vvv-------------------------------------------------------------------- */

#if defined(VBAT_ADC1_SENSE_PIN)

#if 0

hints for selecting ADC channel aka GPIO for ADC
------------------------------------------------

The ESP32-S3 has ADC1 only (no ADC2 like older ESP32). Each ADC channel is hardwired to specific GPIOs.

Typical mapping (ADC1):
GPIO    ADC Channel
GPIO1   ADC1_CH0
GPIO2   ADC1_CH1
GPIO3   ADC1_CH2    <== bat monitor on SUPER MINI
GPIO4   ADC1_CH3
GPIO5   ADC1_CH4
GPIO6   ADC1_CH5
GPIO7   ADC1_CH6
GPIO8   ADC1_CH7
GPIO9   ADC1_CH8
GPIO10  ADC1_CH9


For plain ESP32:

ADC1 (recommended, ADC1 is safe to use even when Wi-Fi is active) 
//GPIO34–39 are input-only (perfect for ADC) This is the best choice for stable measurements:
// <-- without // arduino f.... up?!?!? even within #if 0 ???

GPIO    ADC Channel
GPIO36  ADC1_CHANNEL_0  input only <== standard for ultra_remote    == SENSOR_VP
GPIO37  ADC1_CHANNEL_1  input only
GPIO38  ADC1_CHANNEL_2  input only
GPIO39  ADC1_CHANNEL_3  input only                                  == SENSOR_VN
GPIO32  ADC1_CHANNEL_4
GPIO33  ADC1_CHANNEL_5
GPIO34  ADC1_CHANNEL_6  input only
GPIO35  ADC1_CHANNEL_7  input only


ADC2 (Wi-Fi conflict!), ADC2 cannot be reliably used when Wi-Fi is running (hardware limitation):

GPIO    ADC Channel
GPIO4   ADC2_CHANNEL_0
GPIO0   ADC2_CHANNEL_1
GPIO2   ADC2_CHANNEL_2
GPIO15  ADC2_CHANNEL_3
GPIO13  ADC2_CHANNEL_4
GPIO12  ADC2_CHANNEL_5
GPIO14  ADC2_CHANNEL_6
GPIO27  ADC2_CHANNEL_7
GPIO25  ADC2_CHANNEL_8
GPIO26  ADC2_CHANNEL_9


hints for choosing VBAT_ADC1_ATTENUATION
----------------------------------------
 
values derived from ESP32_2:

from running "adc_info" command: 
ADC VRef calibration: 1114mV

test samples:
1035mv at ADC_ATTEN_DB_0         ->  raw: 4047 
1374mv at ADC_ATTEN_DB_2_5       ->  raw: 4086 
1884mv at ADC_ATTEN_DB_6         ->  raw: 4074 
3127mv at ADC_ATTEN_DB_12        ->  raw: 4073 

raw must always stay below 4096 withing targeted input voltage range

#endif

// esp32 defines ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
// esp32c5 defines ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
// esp32s3 defines ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

adc_oneshot_unit_handle_t adc1_unit;
adc_cali_handle_t adc1_cali;

void
vbat_monitor_init()
{
TP05
#if defined(VBAT_ADC1_GND_PIN)
    static gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (_u64)1 << VBAT_ADC1_GND_PIN;
    gpio_config(&io_conf);
    gpio_set_level(VBAT_ADC1_GND_PIN, 0);   // ground the voltage divider
#endif

    static adc_oneshot_unit_init_cfg_t adc1_init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc1_init_cfg, &adc1_unit));
    static adc_oneshot_chan_cfg_t adc1_chan_cfg = {
        .atten = VBAT_ADC1_ATTENUATION,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_unit, VBAT_ADC1_SENSE_PIN, &adc1_chan_cfg));

#if defined(ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED)

    static adc_cali_curve_fitting_config_t adc1_cali_config = {
        .unit_id = ADC_UNIT_1,
        .chan = VBAT_ADC1_SENSE_PIN,
        .atten = VBAT_ADC1_ATTENUATION,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&adc1_cali_config, &adc1_cali));

#elif defined(ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED)

    static adc_cali_line_fitting_config_t adc1_cali_config = {
        .unit_id = ADC_UNIT_1,
//        .chan = VBAT_ADC1_SENSE_PIN,
        .atten = VBAT_ADC1_ATTENUATION,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&adc1_cali_config, &adc1_cali));

#else
#error UNSUPPORTED ADC_CALI_SCHEME
#endif
}

void
vbat_monitor_deinit()
{
TP05
#if defined(ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED)
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(adc1_cali));
#elif defined(ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED)
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(adc1_cali));
#endif
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_unit));
}

#endif  // if defined(VBAT_ADC1_SENSE_PIN)
/* ---^^^--- monitor battery voltage ---^^^-------------------------------------------------------------------- */


/* ---vvv--- acoustic feedback ---vvv-------------------------------------------------------------------------- */

#include "driver/ledc.h"        // required for ledc_set_freq() in mcom.h
#if ESP_ARDUINO_VERSION > ESP_ARDUINO_VERSION_VAL(2,0,17)       // aka since 3.0.4
#include "driver/gptimer.h"     // required for gptimer_handle_t in mcom.h
#endif

#define     BEEP_6000   6000
#define     BEEP_3000   3000
#define     BEEP_2540   2540
#define     BEEP_1300   1300
#define     BEEP_1000   1000
#define     BEEP_SPIKE  BEEP_6000
#define     BEEP_OK     BEEP_2540
#define     BEEP_ERR    BEEP_1000
#define     BEEP_INFO   BEEP_1300
#define     BEEP_OTA    BEEP_3000

// must be available outside BUZZER define
#define BEEP_SPIKE_PULSE_WIDTH        30000
#define BEEP_SINGLE_PULSE_WIDTH      300000
#define BEEP_MULTI_PULSE_WIDTH        70000
#define BEEP_MULTI_PULSE_PAUSE       250000
#define BEEP_PURGE_PULSE                100
#define BEEP_VOLUME                    2000  // aprox. 1/4 duty cycle (if LEDC_TIMER_13_BIT)
#define BEEP_VOLUME_MAX                4095  // 50% == 4095 (loudest), 100% == 8191 (if LEDC_TIMER_13_BIT)
#define BEEP_VOLUME_SPIKE   BEEP_VOLUME_MAX

#if defined(BUZZER)

#define BEEP_QUE_DEPTH 20
#define BEEP_TIMER_QUE_DEPTH 10

typedef struct {
    _u64 count;
} beep_timer_t;

typedef struct {
    _u32 time;
    _u32 freq;
    _u32 duty;
} beep_t;

static _u8 dispatching = 0;
static QueueHandle_t beep_que = 0;
static QueueHandle_t beep_timer_que = 0;
static gptimer_handle_t beep_timer = 0;

void 
issue_beep(_u32 freq, _u32 duty)
{
TP05
    static _u32 was_here;

    if (freq) {
        if (was_here) {
#if defined(CONFIG_IDF_TARGET_ESP32C5) || defined(CONFIG_IDF_TARGET_ESP32S3)
            ESP_ERROR_CHECK(ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq));
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty));  // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#else
            ESP_ERROR_CHECK(ledc_set_freq(LEDC_HIGH_SPEED_MODE, LEDC_TIMER_0, freq));
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty));  // Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
#endif
        } else {
            ledc_timer_config_t ledc_timer = {
#if defined(MCOM_ARD)
                .speed_mode       = LEDC_HIGH_SPEED_MODE,
                .duty_resolution  = LEDC_TIMER_13_BIT,
                .timer_num        = LEDC_TIMER_0,
                .freq_hz          = freq,
                .clk_cfg          = LEDC_AUTO_CLK
#else   // defined(MCOM_ARD)
#if defined(CONFIG_IDF_TARGET_ESP32C5) || defined(CONFIG_IDF_TARGET_ESP32S3)
                .speed_mode       = LEDC_LOW_SPEED_MODE,
#else
                .speed_mode       = LEDC_HIGH_SPEED_MODE,
#endif
                .timer_num        = LEDC_TIMER_0,
                .duty_resolution  = LEDC_TIMER_13_BIT,
                .freq_hz          = freq,
/*
 * NOT LEDC_AUTO_CLK here. auto selection runs ONCE, on this first config, and ledc_set_freq()
 * later reads the clock back and reuses it (see ledc_set_freq() in esp_driver_ledc/src/ledc.c)
 * -> the first tone locks the clock in. at 13 bit a tone needs freq * 8192 of source clock:
 *      BEEP_INFO  1300Hz -> 10.65MHz   (RC_FAST 17.5MHz is enough -> auto may settle there)
 *      BEEP_SPIKE 6000Hz -> 49.15MHz   (only PLL_F80M/APB 80MHz can do this)
 * so a low first tone strands every later high tone -> ESP_FAIL -> ESP_ERROR_CHECK -> abort.
 * pinning the fastest source keeps all of BEEP_1000..BEEP_6000 reachable (ceiling 9765Hz)
 * and leaves the 13 bit duty scale (BEEP_VOLUME et.al.) untouched
 */
#if defined(CONFIG_IDF_TARGET_ESP32C5)
                .clk_cfg          = LEDC_USE_PLL_DIV_CLK    // PLL_F80M
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
                .clk_cfg          = LEDC_USE_APB_CLK        // 80MHz, same ceiling
#else
                .clk_cfg          = LEDC_AUTO_CLK
#endif
#endif  // defined(MCOM_ARD)
            };
            ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
            ledc_channel_config_t ledc_channel = {
#if defined(MCOM_ARD)
                .gpio_num       = BUZZER,
                .speed_mode     = LEDC_HIGH_SPEED_MODE,
                .channel        = LEDC_CHANNEL_0,
                .intr_type      = LEDC_INTR_DISABLE,
                .timer_sel      = LEDC_TIMER_0,
                .duty           = duty,
#else   // defined(MCOM_ARD)
#if defined(CONFIG_IDF_TARGET_ESP32C5) || defined(CONFIG_IDF_TARGET_ESP32S3)
                .speed_mode     = LEDC_LOW_SPEED_MODE,
#else
                .speed_mode     = LEDC_HIGH_SPEED_MODE,
#endif
                .channel        = LEDC_CHANNEL_0,
                .timer_sel      = LEDC_TIMER_0,
                .intr_type      = LEDC_INTR_DISABLE,
                .gpio_num       = BUZZER,
                .duty           = duty,
#endif  // defined(MCOM_ARD)
            };
            ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
            ++was_here;
        }
#if defined(CONFIG_IDF_TARGET_ESP32C5) || defined(CONFIG_IDF_TARGET_ESP32S3)
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0));
#else
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0));
#endif
    } else {
        if (was_here) {
#if defined(CONFIG_IDF_TARGET_ESP32C5) || defined(CONFIG_IDF_TARGET_ESP32S3)
            ESP_ERROR_CHECK(ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0));
#else
            ESP_ERROR_CHECK(ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0));
#endif
        }
    }
}

void 
dispatch_beeps(void *arg)
{
TP05
    beep_t beep_spec;

    while (1) {
        dispatching = 0; 
        if (xQueueReceive(beep_que, &beep_spec, portMAX_DELAY)) {
            ++dispatching;
            PR06("dispatch_beeps got %d %d %d\n", beep_spec.time, beep_spec.freq, beep_spec.duty);
            gptimer_alarm_config_t alrm = {
                .alarm_count = beep_spec.time,
            };
            ESP_ERROR_CHECK(gptimer_set_alarm_action(beep_timer, &alrm));
            ESP_ERROR_CHECK(gptimer_set_raw_count(beep_timer, 0));
            ESP_ERROR_CHECK(gptimer_start(beep_timer));
            issue_beep(beep_spec.freq, beep_spec.duty);
            beep_timer_t dummy;
            xQueueReceive(beep_timer_que, &dummy, portMAX_DELAY);
        }
    }
}

bool 
IRAM_ATTR beep_timer_alrm(gptimer_handle_t beep_timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t beep_timer_que = (QueueHandle_t)user_data;
    gptimer_stop(beep_timer);
    beep_timer_t dummy = {
        .count = edata->count_value
    };
    xQueueSendFromISR(beep_timer_que, &dummy, &high_task_awoken);
    return (high_task_awoken == pdTRUE);
}

void
beep_sync()
{
TP05
    beep_t b_dummy;

    while (xQueuePeek(beep_que, &b_dummy, 0)) {
        vTaskDelay(pdMS_TO_TICKS(10));
        PR06("+");
    }
    while (dispatching) {
        vTaskDelay(pdMS_TO_TICKS(10));
        PR06("-");
    }
}

void
beep_enque(_i32 time, _i32 frequ, _i32 duty)
{
TP05
    beep_t beep_spec;

    beep_spec.time = time;
    beep_spec.freq = frequ;
    beep_spec.duty = duty;
    xQueueSend(beep_que, &beep_spec, 0);
}

void
beep(_u32 frequ, _u32 cnt)
{
TP05
    PR06("frequ: %d, cnt: %d\n", frequ, cnt);
    if (cnt > 1) {
        --cnt;
        beep_enque(BEEP_MULTI_PULSE_WIDTH, frequ, BEEP_VOLUME);
        while (cnt--) {
            beep_enque(BEEP_MULTI_PULSE_PAUSE, 0, 0);
            beep_enque(BEEP_MULTI_PULSE_WIDTH, frequ, BEEP_VOLUME);
        }
    } else {
        beep_enque(BEEP_SINGLE_PULSE_WIDTH, frequ, BEEP_VOLUME);
    }
    beep_enque(BEEP_PURGE_PULSE, 0, 0);
}

void 
beep_init()
{
TP05
    beep_que = xQueueCreate(BEEP_QUE_DEPTH, _SZ(beep_t));
    if (!beep_que) {
        PR00("creating beep_que failed\n");
        return;
    }
    beep_timer_que = xQueueCreate(BEEP_TIMER_QUE_DEPTH, _SZ(beep_timer_t));
    if (!beep_timer_que) {
        PR00("creating beep_timer_que failed\n");
        return;
    }
    gptimer_config_t beep_timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1tick == 1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&beep_timer_config, &beep_timer));
    gptimer_event_callbacks_t cbs = {
        .on_alarm = beep_timer_alrm,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(beep_timer, &cbs, beep_timer_que));
    ESP_ERROR_CHECK(gptimer_enable(beep_timer));
    xTaskCreate(dispatch_beeps, "dispatch_beeps", 4096, 0, 10, 0);
}

void
beep_deinit()
{
TP05
    ESP_ERROR_CHECK(gptimer_disable(beep_timer));
    ESP_ERROR_CHECK(gptimer_del_timer(beep_timer));
    vQueueDelete(beep_timer_que);
    vQueueDelete(beep_que);
}

#else
void beep(_u32, _u32) {}
void beep_enque(_i32, _i32, _i32) {}
void beep_init() {}
void beep_sync() {}
void beep_deinit() {}
#endif
/* ---^^^--- acoustic feedback ---^^^-------------------------------------------------------------------------- */


/* ---vvv--- RGB c... ---vvv----------------------------------------------------------------------------------- */
#if defined(RGB_GPIO_NUM)

#include "driver/rmt_tx.h"

static rmt_channel_handle_t chan;
static rmt_encoder_handle_t copy_encoder;

#define T0H 4   // 0.4 µs (at 10 MHz → 100 ns ticks)
#define T0L 8   // 0.8 µs
#define T1H 7   // 0.7 µs
#define T1L 6   // 0.6 µs

/*
 * progging RGB illuminates the control LED for a tiny moment and keeps it off after
 */
void 
ws2812_set(_u8 r, _u8 g, _u8 b)
{
TP05
    _u8 grb[3] = { g, r, b };

    rmt_symbol_word_t symbols[24];
    _i32 idx = 0;
    for (_i32 i = 0; i < 3; i++) {
        for (_i32 bit = 7; bit >= 0; bit--) {
            if (grb[i] & (1 << bit)) {
                rmt_symbol_word_t s;
                s.level0 = 1; 
                s.duration0 = T1H;
                s.level1 = 0; 
                s.duration1 = T1L;
                symbols[idx++] = s;
            } else {
                rmt_symbol_word_t s;
                s.level0 = 1; 
                s.duration0 = T0H;
                s.level1 = 0; 
                s.duration1 = T0L;
                symbols[idx++] = s;
            }
        }
    }
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };
    rmt_transmit(chan, copy_encoder, symbols, sizeof(symbols), &tx_config);
    rmt_tx_wait_all_done(chan, portMAX_DELAY);
}

/*
 * initing RGB illuminates the control LED half dim for the time of progging
 * and keeps it off even after shutdown (AKA when sleeping)
 * this implies GPIO remains an output and is low?!
 */
void 
ws2812_init(void)
{
TP05
    rmt_tx_channel_config_t config = {
        .gpio_num = (gpio_num_t)RGB_GPIO_NUM,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10000000, // 10 MHz
        .mem_block_symbols = 64,
        .trans_queue_depth = 1,
    };
    rmt_new_tx_channel(&config, &chan);
    rmt_enable(chan);
    rmt_copy_encoder_config_t copy_config = {};
    rmt_new_copy_encoder(&copy_config, &copy_encoder);
//    ws2812_set(0, 0, 0);  // initial state off
}
#endif  // if defined(RGB_GPIO_NUM)
/* ---^^^--- RGB c... ---^^^----------------------------------------------------------------------------------- */


/* ---vvv--- LED c... ---vvv----------------------------------------------------------------------------------- */
#if defined(LED_GPIO_NUM)
void 
led_init()          
{
TP05
    gpio_config_t io_conf = {};
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask |= 1ULL << LED_GPIO_NUM;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);
//    led_act(0);    // initial state off
}
#endif  // if defined(LED_GPIO_NUM)
/* ---^^^--- LED c... ---^^^----------------------------------------------------------------------------------- */


#if 0
===================== ACCESSPOINTS AS OF 2026_03_03 (authoritative here!!) =====================

check with [ iw wlan0 station dump | grep Station | wc ] for utilization grade

host13   name_accesspoint2       pass_accesspoint4...    5G  ROTA5G_SSID # <-- chan 36 C5 only
host11  name_accesspoint2       pass_accesspoint4...    5G  ROTA5G_SSID # <-- chan 36 C5 only
host12  name_accesspoint2       pass_accesspoint4...    5G  ROTA5G_SSID # <-- chan 36 C5 only
host7 name_accesspoint2       pass_accesspoint4...    5G  ROTA5G_SSID # <-- chan 36 C5 only      
host8 name_accesspoint2       pass_accesspoint4...    5G  ROTA5G_SSID # <-- chan 36 C5 only      
host9 name_accesspoint2      pass_accesspoint2...    2G  ROTA2I_SSID # <-- chan 11 for test devices "study" / ALSO MICRO_AP!!!
host12  name_accesspoint3      pass_accesspoint2...    2G  ROTA2K_SSID # <-- chan  6 for permanent devices "mediaroom"
ufire name_accesspoint2       pass_accesspoint4...    2G  UFIRE_SSID  # <-- ufire
u2fire name_accesspoint7     pass_accesspoint4...    5G  U2FIRE_SSID # <-- u2fire
host5 name_accesspoint2       pass_accesspoint4...    5G  ROTA5G_SSID # <-- ufire
sfire name_accesspoint5       pass_accesspoint5...    2G  SFIRE_SSID  # <-- sfire 
      name_accesspoint5                        5G              # <-- sfire 
kfire name_accesspoint1 pass_accesspoint1...    2G  KFIRE_SSID  # <-- kfire
host10  name_accesspoint6      pass_accesspoint2...    2G  TETHER_SSID # <-- wireless hotspot
===================== ACCESSPOINTS AS OF 2025_04_19 (authoritative here!!) =====================
#endif


/* ---vvv--- WiFi section (accesspts + helpers) ---vvv--------------------------------------------------------- */
                        // 
                        // don't change ordering for DHCP data recorded: USED AS INDEX into _i8p accpts[] below
#define NA_SSID 0       //    
#define U2FIRE_SSID 1   //    
#define ROTA2I_SSID 2   //    
#define TETHER_SSID 5   //      
#define SFIRE_SSID 6    //    
#define UFIRE_SSID 7    //    
#define KFIRE_SSID 8    //    
#define ROTA5G_SSID 9   //    
#define ROTA2K_SSID 10  //    
#define ESPNOW_SSID 11  // micro access point

#define GET_SSID(indx) (accpts[((indx) << 1) + 0])
#define GET_PASS(indx) (accpts[((indx) << 1) + 1])

#define CC2STR(a) (a)[0], (a)[1], (a)[2]
#define CCSTR "%c%c%c"

_i8cp accpts[] = {
    NA_SSID_STR,        NA_PASSWORD_STR,        //  0                 
    U2FIRE_SSID_STR,    U2FIRE_PASSWORD_STR,    //  1  
    ROTA2I_SSID_STR,    ROTA2I_PASSWORD_STR,    //  2  
    0,    0,                                    //  3   // place holder
    0,    0,                                    //  4   // place holder
    TETHER_SSID_STR,    TETHER_PASSWORD_STR,    //  5             
    SFIRE_SSID_STR,     SFIRE_PASSWORD_STR,     //  6            
    UFIRE_SSID_STR,     UFIRE_PASSWORD_STR,     //  7            
    KFIRE_SSID_STR,     KFIRE_PASSWORD_STR,     //  8            
    ROTA5G_SSID_STR,    ROTA5G_PASSWORD_STR,    //  9             
    ROTA2K_SSID_STR,    ROTA2K_PASSWORD_STR,    // 10             
    ESPNOW_SSID_STR,    ESPNOW_PASSWORD_STR,    // 11             
};

#if defined(ATTENTION_REDUCED_WIFI_POWER)
    #define REDUCE_WIFI_POWER_IF_REQUIRED() \
            PR02("reducing TX power down to %d on %s\n", ATTENTION_REDUCED_WIFI_POWER, DEVICE_FW); \
            ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(ATTENTION_REDUCED_WIFI_POWER))
#else
    #define REDUCE_WIFI_POWER_IF_REQUIRED() 
#endif


#if defined(DUMP_SOME)

void
dump_cfg(const _i8 *str, wifi_init_config_t *cfg)
{
    PR00("---v--- %s ---v---\n", str);
    GV05(cfg->static_rx_buf_num);
    GV05(cfg->dynamic_rx_buf_num);
    GV05(cfg->tx_buf_type);
    GV05(cfg->static_tx_buf_num);
    GV05(cfg->dynamic_tx_buf_num);
    GV05(cfg->cache_tx_buf_num);
    GV05(cfg->csi_enable);
    GV05(cfg->ampdu_rx_enable);
    GV05(cfg->ampdu_tx_enable);
    GV05(cfg->amsdu_tx_enable);
    GV05(cfg->nvs_enable);
    GV05(cfg->nano_enable);
    GV05(cfg->rx_ba_win);
    GV05(cfg->wifi_task_core_id);
    GV05(cfg->beacon_max_len);
    GV05(cfg->mgmt_sbuf_num);
    GW05(cfg->feature_caps);
    GV05(cfg->sta_disconnected_pm);
    GV05(cfg->espnow_max_encrypt_num);
    GV05(cfg->magic);
    PR00("---^--- %s ---^---\n", str);
}

void
dump_ev(const _i8 *str, ip_event_got_ip_t *ev)
{
    PR00("---v--- %s ---v---\n", str);
    PR00("ip:   " IPSTR "\n", IP2STR(&ev->ip_info.ip));
    PR00("mask: " IPSTR "\n", IP2STR(&ev->ip_info.netmask));
    PR00("gw:   " IPSTR "\n", IP2STR(&ev->ip_info.gw));
    PR00("---^--- %s ---^---\n", str);
}

// wifi_mode:
//  WIFI_MODE_STA
//  WIFI_MODE_AP
// must have already started wifi for that/ ard-esp32: must be called after WiFi.begin()
void
dump_some(_i32 wifi_mode, const _i8 *str)
{

#if defined(MCOM_ARD)

//calling esp_wifi_sta_get_ap_info() provides we already received:
//WiFi Event [ 510 ] Connected to access point
//the assertion fails otherwise
while (WiFi.status() != WL_CONNECTED) {
    PR00(".");
    vTaskDelay(pdMS_TO_TICKS(30));
}
vTaskDelay(pdMS_TO_TICKS(100));

#endif  // defined(MCOM_ARD)

    PR00("---v--- %s (%s)---v---\n", str, give_mode(wifi_mode));
    _i8 country[20];
    ESP_ERROR_CHECK(esp_wifi_get_country_code(country));
    PR00("val: country.cc[]: <" CCSTR ">\n", CC2STR(country));

    wifi_country_t w_country;
    ESP_ERROR_CHECK(esp_wifi_get_country(&w_country));
    PR00("val: w_country.cc[]: <" CCSTR ">\n", CC2STR(w_country.cc));
    GV05(w_country.schan);
    GV05(w_country.nchan);
    GV05(w_country.max_tx_power);
    GV05(w_country.policy);

#if 0
typical values:

w_country.cc[]: <01 >
val: w_country.schan == 0x1
val: w_country.nchan == 0xb
val: w_country.max_tx_power == 0x14
val: w_country.policy == 0x0
val: power == 0x4e              # <== unit is 0.25dBm -> 0x4e==78, 78 * 0.25 == 19.50dBm

country.cc[]: <DE >
w_country.cc[]: <DE >
val: w_country.schan == 0x1
val: w_country.nchan == 0xd
val: w_country.max_tx_power == 0x14
val: w_country.policy == 0x0
val: power == 0x4e              # <== unit is 0.25dBm -> 0x4e==78, 78 * 0.25 == 19.50dBm

country.cc[]: <US >
w_country.cc[]: <US >
val: w_country.schan == 0x1
val: w_country.nchan == 0xb
val: w_country.max_tx_power == 0x1e
val: w_country.policy == 0x0
val: power == 0x4e              # <== unit is 0.25dBm -> 0x4e==78, 78 * 0.25 == 19.50dBm
#endif

    int8_t power;
    ESP_ERROR_CHECK(esp_wifi_get_max_tx_power(&power)); //  unit is 0.25dBm
    GV05(power);

    wifi_mode_t mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&mode));
    PR00("mode: %s\n", give_mode(mode));

    wifi_ps_type_t type;
    ESP_ERROR_CHECK(esp_wifi_get_ps(&type));
    PR00("ps:   %s\n", give_ps(type));

// because of C5:  
//      esp_wifi_get_bandwidth -> esp_wifi_get_bandwidths
    wifi_bandwidths_t bw;
    ESP_ERROR_CHECK(esp_wifi_get_bandwidths(wifi_mode == WIFI_MODE_STA ? WIFI_IF_STA : WIFI_IF_AP, &bw));
    PR00("bw2G: %s\n", give_bw(bw.ghz_2g));
    PR00("bw5G: %s\n", give_bw(bw.ghz_5g));

    __u32 ev_mask;
    ESP_ERROR_CHECK(esp_wifi_get_event_mask(&ev_mask));    // dflt WIFI_EVENT_MASK_AP_PROBEREQRECVED
    GV05(ev_mask);

if (wifi_mode == WIFI_MODE_STA) {
    // Get information of AP to which the device is associated with.
    wifi_ap_record_t ap_info;
    ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));
    PR00("val: ap_info.bssid " MACSTR "\n", MAC2STR(ap_info.bssid));
    GS05(ap_info.ssid);
    GV05(ap_info.primary);
    GV05(ap_info.second);
    GV05(ap_info.rssi);
    GV05(ap_info.authmode);
    GV05(ap_info.pairwise_cipher);
    GV05(ap_info.group_cipher);
    GV05(ap_info.ant);
    GV05(ap_info.phy_11b);
    GV05(ap_info.phy_11g);
    GV05(ap_info.phy_11n);
    GV05(ap_info.phy_lr);
    GV05(ap_info.phy_11ax);
    GV05(ap_info.wps);
    GV05(ap_info.ftm_responder);
    GV05(ap_info.ftm_initiator);
    GV05(ap_info.reserved);
    PR00("val: ap_info.country[]: <" CCSTR ">\n", CC2STR(ap_info.country.cc));
    GV05(ap_info.country.schan);
    GV05(ap_info.country.nchan);
    GV05(ap_info.country.max_tx_power);
    GV05(ap_info.country.policy);
    GV05(ap_info.he_ap.bss_color);
    GV05(ap_info.he_ap.partial_bss_color);
    GV05(ap_info.he_ap.bss_color_disabled);
    GV05(ap_info.he_ap.bssid_index);
}
if (wifi_mode == WIFI_MODE_STA) {
    // Get configuration of specified interface
    wifi_config_t conf;
    ESP_ERROR_CHECK(esp_wifi_get_config(wifi_mode == WIFI_MODE_STA ? WIFI_IF_STA : WIFI_IF_AP, &conf));
    GS05(conf.sta.ssid);
    GS05(conf.sta.password);
    GV05(conf.sta.scan_method);
    GV05(conf.sta.bssid_set);
    PR00("val: conf.sta.bssid " MACSTR "\n", MAC2STR(conf.sta.bssid));
    GV05(conf.sta.channel);
    GV05(conf.sta.listen_interval);
    GV05(conf.sta.sort_method);
    GV05(conf.sta.threshold.rssi);
    GV05(conf.sta.threshold.authmode);
    GV05(conf.sta.pmf_cfg.capable);
    GV05(conf.sta.pmf_cfg.required);
    GV05(conf.sta.rm_enabled);
    GV05(conf.sta.btm_enabled);
    GV05(conf.sta.mbo_enabled);
    GV05(conf.sta.ft_enabled);
    GV05(conf.sta.owe_enabled);
    GV05(conf.sta.transition_disable);
    //GV05(conf.sta.reserved);  no longer in 5.5.4
    GV05(conf.sta.sae_pwe_h2e);
    GV05(conf.sta.sae_pk_mode);
    GV05(conf.sta.failure_retry_cnt);
    GV05(conf.sta.he_dcm_set);
    GV05(conf.sta.he_dcm_max_constellation_tx);
    GV05(conf.sta.he_dcm_max_constellation_rx);
    GV05(conf.sta.he_mcs9_enabled);
    GV05(conf.sta.he_su_beamformee_disabled);
    GV05(conf.sta.he_trig_su_bmforming_feedback_disabled);
    GV05(conf.sta.he_trig_mu_bmforming_partial_feedback_disabled);
    GV05(conf.sta.he_trig_cqi_feedback_disabled);
    //GV05(conf.sta.he_reserved);  no longer in 5.5.4
} else {
    // Get configuration of specified interface
    wifi_config_t conf;
    ESP_ERROR_CHECK(esp_wifi_get_config(wifi_mode == WIFI_MODE_STA ? WIFI_IF_STA : WIFI_IF_AP, &conf));
    GS05(conf.ap.ssid);
    GS05(conf.ap.password);
    GV05(conf.ap.ssid_len);
    GV05(conf.ap.channel);
    GV05(conf.ap.authmode);
    GV05(conf.ap.ssid_hidden);
    GV05(conf.ap.max_connection);
    GV05(conf.ap.beacon_interval);
    GV05(conf.ap.csa_count);
    GV05(conf.ap.dtim_period);
    GV05(conf.ap.pairwise_cipher);
    GV05(conf.ap.ftm_responder);
//    GV05(conf.ap.pmf_cfg);
    GV05(conf.ap.sae_pwe_h2e);
}
    PR00("---^--- %s (%s)---^---\n", str, give_mode(wifi_mode));
}

void
ur_print_all_netif_ips(const _i8 *prefix)
{
TP05
    extern bool ur_is_our_netif(const _i8 *, esp_netif_t *);

    // iterate over active interfaces, and print out IPs of "our" netifs
    esp_netif_t *netif = 0;
    for (_i32 i = 0; i < esp_netif_get_nr_of_ifs(); ++i) {
        netif = esp_netif_next_unsafe(netif);
        if (ur_is_our_netif(prefix, netif)) {
            PR00("Connected to %s\n", esp_netif_get_desc(netif));
            esp_netif_ip_info_t ip;
            ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip));
            PR00("  ip:      " IPSTR "\n", IP2STR(&ip.ip));
            PR00("  netmask: " IPSTR "\n", IP2STR(&ip.netmask));
            PR00("  gw:      " IPSTR "\n", IP2STR(&ip.gw));

            esp_netif_dns_info_t dns;
            ESP_ERROR_CHECK(esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns));
            PR00("  dns:     " IPSTR "\n", IP2STR(&dns.ip.u_addr.ip4));
        }
    }
}
#endif  // if defined(DUMP_SOME)
/* ---^^^--- WiFi section (accesspts + helpers) ---^^^--------------------------------------------------------- */


/* ---vvv--- ETHERNET section ---vvv--------------------------------------------------------------------------- */
/*
 * eth support for LAN8720 PHY
 * 
 * ETH_MODE variants:
 */
// dynamic operation modes
#define ETH_WORKS_AS_DHCP_CLIENT 1
#define ETH_WORKS_AS_AP_BRIDGE 2

// main functionality
#define ETH_OPMODE_NONE 0
#define ETH_OPMODE_AP 1
#define ETH_OPMODE_ETH 2
#define ETH_OPMODE_WIFI 3

#if ETH_OPMODE == ETH_OPMODE_AP || ETH_OPMODE == ETH_OPMODE_ETH || defined(FW_UPGRADE_VIA_ETH)
#include "esp_eth.h"

static _u32 eth_dynamic_mode = 0;
#if ETH_OPMODE == ETH_OPMODE_AP
static bool s_ethernet_is_connected = 0;
#endif
static esp_eth_handle_t s_eth_handle = 0;
static SemaphoreHandle_t s_wait_ip_addr = 0;

void
eth_event_handler(void *arg, esp_event_base_t event_base,
                              __i32 event_id, void *event_data)
{
TP05
    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        PR00("ethernet link up\n");
// ETH_OPMODE == ETH_OPMODE_AP may also run in eth_dynamic_mode != ETH_WORKS_AS_AP_BRIDGE (FW upgrade et.al.)
// so static + dynamic ifs required
#if ETH_OPMODE == ETH_OPMODE_AP
if (eth_dynamic_mode == ETH_WORKS_AS_AP_BRIDGE) {   
        s_ethernet_is_connected = 1;
        _u8 s_eth_mac[6];

        esp_eth_ioctl(s_eth_handle, ETH_CMD_G_MAC_ADDR, s_eth_mac);
        esp_wifi_set_mac(WIFI_IF_AP, s_eth_mac);
        ESP_ERROR_CHECK(esp_wifi_start());
}
#endif
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        PR00("ethernet link down\n");
#if ETH_OPMODE == ETH_OPMODE_AP
if (eth_dynamic_mode == ETH_WORKS_AS_AP_BRIDGE) {   
        s_ethernet_is_connected = 0;
        ESP_ERROR_CHECK(esp_wifi_stop());
}
#endif
        if (s_wait_ip_addr) {
            PR02("xSemaphoreGive(s_wait_ip_addr)\n");
            xSemaphoreGive(s_wait_ip_addr);
        }
        break;
    case ETHERNET_EVENT_START:
        PR00("ethernet started\n");
        break;
    case ETHERNET_EVENT_STOP:
        PR00("ethernet stopped\n");
        if (s_wait_ip_addr) {
            PR02("xSemaphoreGive(s_wait_ip_addr)\n");
            xSemaphoreGive(s_wait_ip_addr);
        }
        break;
    default:
        break;
    }
}

void
got_ip_event_handler(void *arg, esp_event_base_t event_base, __i32 event_id, void *event_data)
{
TP05
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    PR00("Got IP Address: " IPSTR "\n", IP2STR(&event->ip_info.ip));
    if (s_wait_ip_addr) {
        PR02("xSemaphoreGive(s_wait_ip_addr)\n");
        xSemaphoreGive(s_wait_ip_addr);
    }
}

//
// 2 methods to include the proper ETH driver code:
//
//    1. define _INIT_ETH_LAN8720_ONLY_   # and be happy (works for LAN8720 only)
//    2. the IDF_COMPONENT way (using do NOT define _INIT_ETH_LAN8720_ONLY_)
//        base components for the generalized version of example_eth_init() to work:
//        /home/toh/esp-idf.v5.5/components/esp_eth                               [ /src/phy/esp_eth_phy_lan87xx.c ]
//        /home/toh/esp-idf.v5.5/examples/ethernet/basic/components/ethernet_init [ /ethernet_init.c ]
//
//        base sample user of example_eth_init():
//        /home/toh/esp-idf.v5.5/examples/ethernet/basic/main/ethernet_example_main.c
//
//
// for the generic LAN CFG to work (method 2, i.e. _INIT_ETH_LAN8720_ONLY_ NOT defined) you must provide
//
// 1. a proper buildit.cfg with sth. like IDF_COMPONENT=$(cat << '!'... ethernet_init: path: ${IDF_PATH}/examples/ethernet/basic/components/ethernet_init
// 2. a proper SDK_VERS like: sdkconfig_idf_WAVESHARE_S3ETH_16MBFLASH_PSRAM defining CONFIG_EXAMPLE_USE_W5500=y et.al.
//      sdkconfig.defaults_idf_esp32_8MBFLASH_wireless_tag:CONFIG_EXAMPLE_ETH_PHY_LAN87XX=y
//      sdkconfig.defaults_idf_esp32_8MBFLASH_wireless_tag:CONFIG_EXAMPLE_ETH_PHY_RST_GPIO=-1
//
// so to avoid:
//       panics with: expression: example_eth_init(&eth_handles, &eth_port_cnt) / abort() was called
// for the moment define this:
//

#if defined(_INIT_ETH_LAN8720_ONLY_)
// nothin
#else
#include "ethernet_init.h"  // declaration of example_eth_init() et.al.
#endif

_i32 
init_eth(_u8 mode, bool wait)
{
TP05
    eth_dynamic_mode = mode;
if (wait) {
    s_wait_ip_addr = xSemaphoreCreateBinary();
    ESP_ERROR_CHECK(!s_wait_ip_addr);
}

#if defined(_INIT_ETH_LAN8720_ONLY_)

/*
simplified version of example_eth_init() / taylored to LAN8720 only
*/
// prerequistites not handled espressif orig phys layers
    gpio_reset_pin(GPIO_NUM_16);
    gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_16, 1); // Set GPIO16 high to enable the oscillator
//mac
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    esp32_emac_config.smi_gpio.mdc_num = GPIO_NUM_23;
    esp32_emac_config.smi_gpio.mdio_num = GPIO_NUM_18;
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);
//phy
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = 1;
    phy_config.reset_gpio_num = -1;
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);
//combine
    esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(mac, phy);

    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config, &s_eth_handle));

#else   // #if defined(_INIT_ETH_LAN8720_ONLY_)

#if defined(CONFIG_EXAMPLE_USE_W5500)
    // prerequistites not handled espressif orig phys layers
    // none ATM
#endif
#if defined(CONFIG_EXAMPLE_ETH_PHY_LAN87XX)
    // prerequistites not handled espressif orig phys layers
    gpio_reset_pin(GPIO_NUM_16);
    gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_16, 1); // Set GPIO16 high to enable the oscillator
#endif

    _u8 eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles;
    ESP_ERROR_CHECK(example_eth_init(&eth_handles, &eth_port_cnt));
    if (eth_port_cnt != 1) {
        PR00("can't handle eth_port_cnt != 1 interfaces\n");
        return 1;   // err
    }
    s_eth_handle = eth_handles[0];

#endif  // #if defined(_INIT_ETH_LAN8720_ONLY_)
#if ETH_OPMODE == ETH_OPMODE_AP
if (eth_dynamic_mode == ETH_WORKS_AS_AP_BRIDGE) {   
    // make proc call ref iNSTEAD!!
    extern esp_err_t pkt_eth2wifi(esp_eth_handle_t, _u8p, __u32, void *);
    ESP_ERROR_CHECK(esp_eth_update_input_path(s_eth_handle, pkt_eth2wifi, 0));
    bool eth_promiscuous = 1;
    ESP_ERROR_CHECK(esp_eth_ioctl(s_eth_handle, ETH_CMD_S_PROMISCUOUS, &eth_promiscuous));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler, 0));
} else
#endif
{
    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_ETH();

    ESP_ERROR_CHECK(esp_netif_attach(esp_netif_new(&netif_config), esp_eth_new_netif_glue(s_eth_handle)));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler, 0));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, got_ip_event_handler, 0));
}
    ESP_ERROR_CHECK(esp_eth_start(s_eth_handle));

if (wait) {
    PR02("xSemaphoreTake(s_wait_ip_addr)\n");
    xSemaphoreTake(s_wait_ip_addr, portMAX_DELAY); // effective no timeout
    vSemaphoreDelete(s_wait_ip_addr);
}
    return 0;
}

#endif  // #if ETH_OPMODE == ETH_OPMODE_AP || ETH_OPMODE == ETH_OPMODE_ETH || defined(FW_UPGRADE_VIA_ETH)
/* ---^^^--- ETHERNET section ---^^^--------------------------------------------------------------------------- */


/* ---vvv--- OTA section ---vvv-------------------------------------------------------------------------------- */
#include "regex.h"
#include "esp_ota_ops.h"
#include "esp_https_ota.h"      // required for esp_https_ota_config_t in mcom.h

#ifndef URL_FW_DIR      /* -DURL_FW_DIR=... overrides it, for the http/https A-B */
#define URL_FW_DIR "http://example.com/"
#endif

#ifdef OTA_MTLS
/*
 * client certificate + our own CA root, embedded by the project's main/CMakeLists.txt via
 * EMBED_TXTFILES. only a project that actually embeds them may define OTA_MTLS - every other
 * project keeps linking exactly as before
 */
extern const char ota_ca_crt_start[]     asm("_binary_ca_crt_start");
extern const char ota_client_crt_start[] asm("_binary_device_crt_start");
extern const char ota_client_key_start[] asm("_binary_device_key_start");
#define OTA_MTLS_FIELDS                          \
        .cert_pem        = ota_ca_crt_start,     \
        .client_cert_pem = ota_client_crt_start, \
        .client_key_pem  = ota_client_key_start,
#else
#define OTA_MTLS_FIELDS
#endif
#define SERNO_ strtol(SERNO, 0, 16)

/*
 * typical input format: 
 *      href="ultra-tester-2-e7a2.bin"
 */
#define REGEX_STR "href=\"(%s-%d-([0-9a-f]+).bin)\""
#define REGEX_FULL_MATCH 0
#define REGEX_FILE_NAME  1
#define REGEX_SERNO      2
#define REGEX_SIZE (REGEX_SERNO + 1)    // last + 1 == nr of all result matches

/*
 * must define this here due to ur_connect()
 */
#define WIFI_CONN_FAST_FAIL             1   // ultra fast fail after 1 failing retry for ultra remotes
#define WIFI_CONN_MODERATE_FAIL         2   // allow 2 retries -> appears to survive most common pitfalls
#define WIFI_CONN_SLOW_FAIL    0xffffffff   // max uint, wait virtually forever -> survives even the worst (mostly)
#define WIFI_CONN_WAIT                  1   // wait for IP, to gain performance through parallelism 
                                            // not required for mysend() if following the ur_connect()
                                            // as mysend() accounts for DHCP not yet finished
_i32 
find_latest_firmware(regex_t *regex_fw_p, _i8p inbuf, _i8p fw_name)
{
//TP05
    _i32 err;
    _i8 _buf[32];
    regmatch_t regex_fw_match[REGEX_SIZE];      // nr of parenthesized subexprs + 1

    *fw_name = 0;
//do {  // first fits
    if (err = regexec(regex_fw_p, inbuf, _NE(regex_fw_match), regex_fw_match, 0)) {
        regerror(err, regex_fw_p, _buf, _SZ(_buf));
//PR00("regerror: %s\n", _buf);
        *_buf = 0;  // breaks while
    } else {
        *fw_name = 0;
        strncat(fw_name, 
                 inbuf + regex_fw_match[REGEX_FILE_NAME].rm_so, 
                 regex_fw_match[REGEX_FILE_NAME].rm_eo - regex_fw_match[REGEX_FILE_NAME].rm_so);
        *_buf = 0;
//notneed        strcat(_buf, "0x");
        strncat(_buf, 
                 inbuf + regex_fw_match[REGEX_SERNO].rm_so, 
                 regex_fw_match[REGEX_SERNO].rm_eo - regex_fw_match[REGEX_SERNO].rm_so);
        inbuf += regex_fw_match[REGEX_FULL_MATCH].rm_eo;
//PR00("FW file name: %s serno: %s\n", fw_name, _buf);
    }
//} while (*_buf);
    return strtol(_buf, 0, 16);
}

/*
 * 2 possibilities to receive directory listings (depending on response length):
 * either
 * HTTP_EVENT_ON_HEADER, key=Transfer-Encoding, value=chunked      for lengthy responses
 * or
 * HTTP_EVENT_ON_HEADER, key=Content-Length, value=7297            short responses fitting small buffers
 * 
 * 2 ways to solve chunked data transfer:
 * either
 * esp_http_client_fetch_headers() (blocking API)                  <- CHOOSEN METHOD HERE
 * or
 * esp_http_client_perform() (event-driven API per _http_event_handler())
 */
_i32
get_latest_firmware(_i8p fw_name)
{
    esp_http_client_config_t config = {
        .url = URL_FW_DIR,
        .user_agent = DEVICE_FW,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 4000,
        OTA_MTLS_FIELDS
    };
    regex_t regex_fw;
    _i32 err;
    _i8 http_buf[256];  // tmp buffer for reading chunked http responses
    _i8 linebuf[512];   // assemble line after line out of tmp buffer and parse contents
    _i32 linepos = 0;
    _i32 fw_serial = 0;

    sprintf(linebuf, REGEX_STR, PROJECT, ENTITY);
    if (err = regcomp(&regex_fw, linebuf, REG_EXTENDED)) {
        regerror(err, &regex_fw, linebuf, _SZ(linebuf));
        PR00("regerror: %s\n", linebuf);
//      goto out;   // replaces goto out: because of ARDUIONO K....
    } else {        // replaces goto out: because of ARDUIONO K....
    esp_http_client_handle_t client = esp_http_client_init(&config);
#if 0
    //
    // for event-driven API continue from here per handler like this:
    // according to: https://docs.espressif.com/projects/esp-techpedia/en/latest/esp-friends/get-started/case-study/protocols-examples/http-examples/general-steps.html#chunk-encoding
    //
    esp_err_t err = esp_http_client_perform(client);    // <- process data in handler properly
    if (err == ESP_OK) {
        PR00("HTTP chunk encoding Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        PR00("Error perform http request %s", esp_err_to_name(err));
    }

    //
    // or for blocking API continue like this:
    //
#endif
    ESP_ERROR_CHECK(esp_http_client_open(client, 0));
WTPROF("w_idx_open");   // TCP connect + (for https) the whole TLS handshake
    if (esp_http_client_fetch_headers(client)) {        // read (aka discard) headers 
        PR06("processing of non chunked data of known length starts\n");
    } else {
        PR06("processing of chunked data of yet unknown length starts\n");
    }
    while (1) {
        _i32 i;
        _i32 len = esp_http_client_read(client, http_buf, _SZ(http_buf) - 1);
        if (len < 0) {
            PR00("Read error\n");
            break;
        }
        if (len == 0) {
//PR05("end of response\n");
            break;
        }
//PR05("RX %d bytes\n", len);
        http_buf[len] = 0;   // null-terminate for string compatibility
        for (i = 0; i < len; i++) {
            if (http_buf[i] == '\n') {
                linebuf[linepos] = 0;
//PR05("parsing: <%s>\n", linebuf);
                if (fw_serial = find_latest_firmware(&regex_fw, linebuf, fw_name)) {
                    PR06("found FW: %s / serial: %d\n", fw_name, fw_serial);
                    break;
                }
                linepos = 0;
            } else if (linepos < _SZ(linebuf) - 1) {
                linebuf[linepos++] = http_buf[i];
            }
        }
        if (i != len) break;
    }
WTPROF("w_idx_read");   // index body read + regex scan
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    regfree(&regex_fw);
    }   // replaces goto out: because of ARDUIONO K....
//out:  // replaces goto out: because of ARDUIONO K....
    if (!fw_serial) PR06("no firmware image found\n");
    return fw_serial;
}

esp_err_t
_http_event_handler(esp_http_client_event_t *evt)
{
//TP05
    static _u32 sum;
    static _u32 summed;
    static _u32 last;
    _u32 now = tstamp();

    switch (evt->event_id) {
      case HTTP_EVENT_ERROR:
        PR00("HTTP_EVENT_ERROR\n");
        break;
      case HTTP_EVENT_ON_CONNECTED:
        PR06("HTTP_EVENT_ON_CONNECTED\n");
        break;
      case HTTP_EVENT_HEADER_SENT:
        PR06("HTTP_EVENT_HEADER_SENT\n");
        break;
      case HTTP_EVENT_ON_HEADER:
        PR06("HTTP_EVENT_ON_HEADER, key=%s, value=%s\n", evt->header_key, evt->header_value);
        if (!strcmp(evt->header_key, "Content-Length")) {
            PR00("file size to load: %s\n", evt->header_value);
            sum = atoi(evt->header_value);
            summed = 0;
            last = 0;
        }
        break;
      case HTTP_EVENT_ON_DATA:
        //PR06("HTTP_EVENT_ON_DATA (len==%d, chunked==%d)\n", evt->data_len, esp_http_client_is_chunked_response(evt->client));
        if (sum) {
            summed += evt->data_len;
            if (now - last > 1000) {
                PR00("\r %d%%", summed * 100 / sum);
                fflush(stdout);
                last = now;
            }
        }
        break;
      case HTTP_EVENT_ON_FINISH:
        PR00("HTTP_EVENT_ON_FINISH\n");
        break;
      case HTTP_EVENT_DISCONNECTED:
        PR06("HTTP_EVENT_DISCONNECTED\n");
        if (sum) {
            PR00("\r %d%%", summed * 100 / sum);
            PR00("\n");
            sum = 0; // avoid double PR00 on secnd HTTP_EVENT_DISCONNECTED
        }
        break;
#if ESP_ARDUINO_VERSION > ESP_ARDUINO_VERSION_VAL(2,0,17)       // aka since 3.0.4
      case HTTP_EVENT_REDIRECT:
        PR00("HTTP_EVENT_REDIRECT\n");
        break;
#endif
      default:
        PR00("unknown case in http_event_handler\n");
        break;
    }
    return ESP_OK;
}

_i32
myota()
{
TP05
    _i8 url[128];
    _i32 fw_num;
    _i8 fw_name[64];
    _i32 stat = 0;  // default no err

#if defined(MCOM_ARD)
    extern _i32 wait4wifi();
    if (wait4wifi()) {  
        PR00("ERROR: could not OTA (no wifi)\n");
        stat = 1;
        goto out;
    }
#endif  // defined(MCOM_ARD)

    fw_num = get_latest_firmware(fw_name);
    PR00("current FW: %s\n", DEVICE_FW);
    if (fw_num) {
        PR00("avail   FW: %s-%d-%04x\n", PROJECT, ENTITY, fw_num);
    } else {
        PR00("avail   FW: none\n");
    }
    if (!fw_num) {
        PR00("no FW found\n");
    } else if (fw_num == SERNO_) {
        PR00("FW on device is up2date\n");
    } else if (fw_num < SERNO_) {
        PR00("FW on server is outdated\n");
    } else { 
        esp_http_client_config_t config = {
            .user_agent = DEVICE_FW,
            .event_handler = _http_event_handler,
            .keep_alive_enable = true,
            OTA_MTLS_FIELDS
        };
        esp_https_ota_config_t ota_config = {
            .http_config = &config,
        };

        PR00("upgrading FW %s -> %04x\n", SERNO, fw_num);
        beep(BEEP_OTA, 5);
        *url = 0;
        strcat(url, URL_FW_DIR);
        strcat(url, fw_name);        // append name
        config.url = url;
        beep_sync();  // flush to avoid interference with OTA
        PR00("OTA starts\n");
WTPROF("w_ota_start");  // second connection: handshake again, then the image transfer
#if ESP_ARDUINO_VERSION > ESP_ARDUINO_VERSION_VAL(2,0,17)       // aka since 3.0.4
        if (!esp_https_ota(&ota_config)) {
#else
        if (!esp_https_ota(&config)) {  //}
#endif
            PR00("OTA completed\n");
            beep(BEEP_OK, 1);
        } else {
            // a successful OTA will never reach this
            PR00("ERROR: could not OTA (reason unknown)");
            stat = 1;
        }
    }
#if defined(MCOM_ARD)
out:
#endif
    if (stat) {
        beep(BEEP_ERR, 3);
    }
    beep_sync();    // must sync due to imminent restart
    return stat;
}

_i32
auto_upgrade_FW(_i32 forced)
{
TP05
    extern _u32 bootCount;

    _i32 stat = 0;  // per default signal upgrade not attempted and device hardware left untouched (no reboot required)
    __i32 skip_fw_update = 0; /* preset to 0 => value will default to 0 if not set yet in NVS */

    GET_NVS(skip_fw_update);
    if (!skip_fw_update && bootCount == 1 || forced) {
        PR00("firmware update [ initiated ]\n");
        if (!forced) {              // a forced update implies an existing network connection -> so do not create one
#if defined(FW_UPGRADE_VIA_ETH)
            PR06("FW UPGRADE via ETH_CLIENT\n");
            if (init_eth(ETH_WORKS_AS_DHCP_CLIENT, 1)) {
                PR00("could not FW eth_mode client eth lan8720\n");
            }
#else
            PR06("FW UPGRADE via WIFI_CLIENT\n");
#ifdef MCOM_ARD
            extern _i32 init_3rd(_u8, _i8cp);
            if (init_3rd(OTA_SSID, __FILE__)) {
#else
            extern esp_err_t ur_connect(_u8, bool, _u32, _u32);
            if (ur_connect(OTA_SSID, WIFI_CONN_WAIT, WIFI_CONN_SLOW_FAIL, WIFI_PS_NONE)) { //}
#endif
                PR00("could not connect to %s\n", GET_SSID(OTA_SSID));
            }
#endif
        }
        if (myota()) {  // as of 2026_03_15 myota() always returns -> so restart yourself
            PR00("OTA ended with error\n");
        }

        /*
         * for some unknown reasons skip_fw_update is found set to 1 sometimes right after a
         * serial flash?! delaying SET_NVS(skip_fw_update, 1); appears to mitigate the problem
         */
        SET_NVS(skip_fw_update, 1); // after myota() we set this to SKIP in any case
        PR00("firmware update [ ended + disarmed ]\n");
        stat = 1;   // signal an upgrade attempt occured and the device must be rebooted
    } else if (skip_fw_update) {
        SET_NVS(skip_fw_update, 0);
        PR00("firmware update [ skipped + rearmed ]\n");
    }
    return stat;
}
/* ---^^^--- OTA section ---^^^-------------------------------------------------------------------------------- */


/* ---vvv--- cmd to status (regexp defs) ---vvv---------------------------------------------------------------- */
#if defined(WIFI_INITIATOR) || defined(ESPNOW_INITIATOR) || defined(ETH_INITIATOR)

#include "regex.h"

regex_t _regex;
regmatch_t _pmatch[7];  // nr of parenthesized subexprs in STATUS_MATCH + 1

// ---vvv--- STATUS encodings ---vvv---
/*
 * consider
 *  bin/pq.awklib           # for pq-system     [ STATUS_MATCH ]
 *  bin/p                   # for pq-system     [ server_status ]
 *  esp/mcom.h              # for ESP-IDF       [ STATUS_MATCH ]
 *  bin/bell.c              # for door unit     [ STATUS_MATCH ]
 *  local/lib/l7.awklib     # for _MPS scripts  [ STATUS_MATCH ]
 *  bin/tcp_server          # for ESP32 systems [ server_status ]
 *  main/ultra_espnow_gw.c  # for ESPNOW GW     [ server_status ]
 *
 * return  "#["     cmd      "]" \
 *         "#[" !!IS_REPEAT  "]" \
 *         "#[" !!IS_STEALTH "]" \
 *         "#["   PLAY_MODE  "]" \
 *         "#["     misc     "]" \
 *         "#["     stat     "]"
 *
 * also if tampering here. typical stat:
 *    #[GS]#[0]#[0]#[fil]#[ 2.31]#[0]
 */
// ']' is escaped differently in l7.awklib:
//              "^#\\[([^\\]]+)\\]" 
_i8cp STATUS_MATCH =
            "^#\\[([^]]+)\\]" \
                 "#\\[([01])\\]" \
                 "#\\[([01])\\]" \
                 "#\\[([a-z0-9]+)\\]" \
                 "#\\[([^()]+)\\]" \
                 "#\\[([01])\\]$";

// encodings in misc subfield of status (must match pq.awklib):
// 252 (was STATUS_MISC_DUPLICATE) is RETIRED - it existed while the gateways still had to
// translate an error status carrying that marker back into a good one. targets now filter their
// own duplicates and answer them with the same benign status as a real execution, so nothing
// marks a duplicate on the wire any more. do not reintroduce it
#define STATUS_MISC_PLAY_RUN   253       // player running indication

#define STAT_CMD 1     // parenthesized subexprs indices
#define STAT_REPEAT 2
#define STAT_STEALTH 3
#define STAT_PLAY_MODE 4
#define STAT_MISC 5
#define STAT_STAT 6
// ---^^^--- STATUS encodings ---^^^---

_i8cp _err[] = {
    "ok",      // sample text

    // this module
    "err#1",   // no WiFi conn (so no status)   
    "err#2",   // no target conn (so no status)
    "err#3",   // conn but no status          
    "err#4",   // wrong status format        
    "err#5",   // status nok                     

    // others          
    "err#6",   // temp conversion not complete
    "err#7",   // temp not plausible
};

#endif  // if defined(WIFI_INITIATOR) || defined(ESPNOW_INITIATOR) || defined(ETH_INITIATOR)
/* ---^^^--- cmd to status (regexp defs) ---^^^---------------------------------------------------------------- */


/* ---vvv--- ESPNOW/ WiFi/ ETH common section ---vvv----------------------------------------------------------- */
#if defined(ESPNOW_TARGET) || defined(ESPNOW_INITIATOR) || defined(WIFI_INITIATOR) || defined(ETH_INITIATOR)

/*
 * common struct to all
 */
#define ESPNOW_DATALEN 128

typedef struct {              
    _u8 data[ESPNOW_DATALEN];
    _u8 len;
    _i8 rssi;
    _u8 src_mac[ESP_NOW_ETH_ALEN];
    _u8 dst_mac[ESP_NOW_ETH_ALEN];
#if defined(GW_TIMING_TRACE)
    /*
     * arrival time in the recv callback, us. the ONLY way to see how long a command sat in
     * espnow_unsol_que: the task cannot ask afterwards when its packet turned up, and a side
     * channel would race the queue. tstamp() is no use here - it is tick based at FREERTOS_HZ
     * 100, i.e. 10ms granular, against a queue wait we expect to be a few ms
     */
    _u64 t_rx;
#endif
} espnow_packet_t;

// stripped down espnow_packet_t with data only for easy espnow_packet_t -> generic_packet_t copy
typedef struct {               
    _u8 data[ESPNOW_DATALEN];
} generic_packet_t;

// required for common code paths ETH, WiFi, ESPNOW in mysend()
espnow_packet_t espnow_sol_pkt;
#endif  // if defined(ESPNOW_TARGET) || defined(ESPNOW_INITIATOR) || defined(WIFI_INITIATOR) || defined(ETH_INITIATOR)

/* ---^^^--- ESPNOW/ WiFi/ ETH common section ---^^^----------------------------------------------------------- */


/* ---vvv--- ESPNOW section ---vvv----------------------------------------------------------------------------- */
#if defined(ESPNOW_TARGET) || defined(ESPNOW_INITIATOR)

#if defined(ESPNOW_INITIATOR)

#define ESPNOW_EVENT_RESPONSE_RECEIVED(bit) (1 << (bit))    // preparation for multi buffer rcv
#define ESPNOW_EVENT_RESPONSE_MASK ( \
                                       ESPNOW_EVENT_RESPONSE_RECEIVED(0) /* all msks combined */ \
                                   )
#define ESPNOW_STATUS_TIMEOUT 1000  // time window should preferredly match status reporting lag of non pre-acked 2-char cmds
                                    // UPDATE as of 2026_09_13: stay at 1000ms to timeout quickly if no connection exists at all
/*
 * ESPNOW_RESEND_UNANSWERED - how often a cmd that drew no answer within ESPNOW_STATUS_TIMEOUT is
 * sent again, unchanged, serial and all. 0, the default, keeps the single attempt. repeating is
 * safe because of the serial: the target executes the first copy it sees and answers any later
 * one as a duplicate with the same status (bell: "sent duplicate"), and tcp_server drops repeats
 * of pre-acked cmds. define it per entity before including this file
 */
#if !defined(ESPNOW_RESEND_UNANSWERED)
#define ESPNOW_RESEND_UNANSWERED 0
#endif

// check for existence of EvGrp to allow ESPNOW_QUE_DEINIT() to be a stub
// due to EvGrps and such can't be deleted easily
#define ESPNOW_QUE_INIT() \
do { \
    if (!espnow_sol_events) { \
        espnow_sol_events = xEventGroupCreate(); \
    } \
} while (0)

#define ESPNOW_QUE_DEINIT()

/* -------------------------------------------------------------------------------------------------------
DELAY against ANTENNA COUNT:
espnow cmd: 96 [np ^host2.example.com:8899] len 20 -> gateway      [ 1 ]
stat: 106 #[XX]#[0]#[0]#[xxx]#[254]#[0], rssi -95                   <== VERIFIED worst case (GW available)

espnow cmd: 96 [no ^host2.example.com:8899 ^25S] len 25 -> gateway [ 2 ]
stat: 136 #[XX]#[0]#[0]#[xxx]#[0]#[0], rssi -90                     <== VERIFIED worst case (GW not available)

espnow cmd: 96 [np ^host2.example.com:8899 ^26S] len 25 -> gateway [ 3 ]
stat: 166 #[XX]#[0]#[0]#[xxx]#[254]#[0], rssi -93                   <== VERIFIED worst case (GW not available)

espnow cmd: 96 [no ^host2.example.com:8899 ^25S] len 25 -> gateway [ 4 ]
stat: 196 #[XX]#[0]#[0]#[xxx]#[0]#[0], rssi -90                     <== VERIFIED worst case (GW not available)

espnow cmd: 96 [no ^host2.example.com:8899 ^24S] len 25 -> gateway [ 5 ]
stat: 246 #[XX]#[0]#[0]#[xxx]#[0]#[0], rssi -89                     <== VERIFIED worst case (GW not available)

espnow cmd: 96 [np ^host2.example.com:8899 ^20S] len 25 -> gateway [ 6 ]
stat: 206 #[XX]#[0]#[0]#[xxx]#[254]#[0], rssi -93

espnow cmd: 96 [np ^host2.example.com:8899 ^40S] len 25 -> gateway [ 7 ]
stat: 276 #[XX]#[0]#[0]#[xxx]#[254]#[0], rssi -95

espnow cmd: 96 [np ^host2.example.com:8899 ^64S] len 25 -> gateway [ 8 ]
stat: 206 #[XX]#[0]#[0]#[xxx]#[254]#[0], rssi -95

espnow cmd: 106 [np ^host2.example.com:8899 ^14S] len 25 -> gateway[ 9 ]
stat: 356 #[XX]#[0]#[0]#[xxx]#[254]#[0], rssi -94
-------------------------------------------------------------------------------------------------------*/

/*
 * ATTENTION:
 *  using NUM_ESPNOW_GATEWAYS > 1 activates serial-no generation
 *  if the cmd is sent pre-acked esp32_decode/tcp_server takes care of serial-no evaluation
 *  if the cmd is sent non-pre-acked the target application must understand serial-no handling
 */
/*
 * ESPNOW_DUT_GW_MAC - slot 1 of the array below, the ONE gateway under test.
 *
 * 2026_09_01, one-at-a-time comparison of the co-located bench gateways 75/81/87. they may NOT be
 * measured simultaneously: with all three in the array they shut each other out, measured as
 * esp32-87 hearing 3% at 20dBm but 6% at 16dBm (n~1716 per level, >7 sigma the WRONG way) - a link
 * budget cannot do that, only the neighbours' own transmissions can. every gateway that hears also
 * answers, so each extra entry is more airtime right at the bench.
 *
 * the array is therefore held at exactly FOUR entries and only this slot changes. slot 1 is where
 * ESPNOW_007 (esp32-75) always sat, so with 007 selected the array is byte-identical to the
 * historic configuration and all three sessions compare to each other AND to the older campaign
 * data. esp32-79 stays in although it is dead weight at this bench (0%): holding the conditions
 * identical across the three sessions is worth more than the retry chain it costs.
 *
 * switch by moving the comment. rebuild and reflash BOTH remotes (45 and 53) for each session.
 *
 * refer to mcfg_local.h for a full gateway list
 */
//#define ESPNOW_DUT_GW_MAC ESPNOW_005_GW_MAC   // esp32-61  TESTING esp32-eth01 wireless-tag clone,
                                                //           plain ESP32, ON-BOARD antenna
  #define ESPNOW_DUT_GW_MAC ESPNOW_007_GW_MAC   // esp32-75  DEFAULT, "bedroom"
//#define ESPNOW_DUT_GW_MAC ESPNOW_010_GW_MAC   // esp32-81  RETIRED 2026_09_01 - withdrawn
                                                //           from the comparison by hand
//#define ESPNOW_DUT_GW_MAC ESPNOW_011_GW_MAC   // esp32-87  TESTING

/*
 * ESPNOW_BENCH_DROP_UNREACHABLE - leave out the array entries this site cannot reach at all.
 *
 * OFF by default, deliberately. the tx power campaign above needs the array held byte-identical
 * across its sessions, and esp32-79 is part of that constant even though it answers 0% here - so
 * uncommenting this in mcom.h would silently change the conditions for every project that defines
 * MULTI_ANTENNA_GATEWAY, the sweep included. define it in the PROJECT instead, ahead of
 * #include "mcom.h", the way ultra_remote does.
 *
 * it exists for the audible-gap hunt on the key press path, where the array is not a constant to
 * be preserved but the thing under investigation. measured on esp32-45 out of the study collector
 * capture, 2026_09_14 16:20..16:50, 1039 commands - air frames per unicast burst, per destination:
 *
 *      esp32-77 study        1.0 mean, ACKed on the first frame 100% of the time
 *      esp32-87 mediaroom    6.2 mean
 *      esp32-75 bedroom     26.2 mean, 78% run into the 32 frame retry ceiling
 *      esp32-79 cellar      31.8 mean, 100% run into it, never ACKed once in 1033 bursts
 *
 * so the cellar entry costs ~32 retries * 1.12ms = ~35ms of transmit airtime on EVERY command,
 * and a single radio is deaf for the duration. the status round trip over the same window is
 * p50 26ms / p90 46ms / max 71ms against a BEEP_SPIKE_PULSE_WIDTH of 30ms - the chirp IS the whole
 * budget for a gapless status tone, and that burst is most of what eats it. RTT tracks burst
 * length almost linearly: 1-2 frames -> 6.7ms mean, 25-32 frames -> 40.5ms mean.
 *
 * only entries that never answer belong here. esp32-75 stays in despite its 78%: it is a real
 * bench gateway, it does win the race back sometimes, and dropping it would change the system
 * under test rather than the measurement of it.
 */
//#define ESPNOW_BENCH_DROP_UNREACHABLE     // per project, NOT here - see above

EventGroupHandle_t espnow_sol_events;
_u8 espnow_gateway_mac[][ESP_NOW_ETH_ALEN] = {

    ESPNOW_GW_MAC,          // site A: esp32-44 #1 botland "mediaroom"  <== still active but DEPRECATED (as of 2026_09_14) / migrating to esp32-86/87
                            // site A: esp32-87 esp32-s3-eth wave share R8MTH4 16MB Flash/ 8MB PSRAM "mediaroom" <= active since 2026_09_14 
                            // site B: esp32-37 #3 botland "host14 garage"

#if defined(MULTI_ANTENNA_GATEWAY)
    ESPNOW_DUT_GW_MAC,      //  the gateway under test - see ESPNOW_DUT_GW_MAC above
    ESPNOW_008_GW_MAC,      //  esp32-77 ultra_espnow_gw/ wifi  esp32-s3-eth wave share R8MTH4 16MB Flash/ 8MB PSRAM "study"         <== control, never changes
#if !defined(ESPNOW_BENCH_DROP_UNREACHABLE)
    ESPNOW_009_GW_MAC,      //  esp32-79 ultra_espnow_gw/ wifi  esp32-s3-eth wave share R8MTH4 16MB Flash/ 8MB PSRAM "cellar"        <== control, never changes
#endif
#endif  

};
#define NUM_ESPNOW_GATEWAYS _NE(espnow_gateway_mac)

#define ESPNOW_ADD_PEERS() \
    for (_u32 i = 0; i < NUM_ESPNOW_GATEWAYS; ++i) { \
        PR02("ESPNOW_ADD_PEER: %d " MACSTR "\n", i, MAC2STR(espnow_gateway_mac[i])); \
        ESPNOW_ADD_PEER(espnow_gateway_mac[i]); \
    }

/*
 * this is for solicited packets e.g. status
 */
void
espnow_recv_cb(const esp_now_recv_info_t *info,
                           const _u8 *data, int len)
{
    /*
     * HACK as of 2026_06_16:
     *    don't update buffer if data already there (aka len != 0)
     *    to avoid overwrites from mutual multi gateway responses
     *    this could happen in PRE-ACKing case as all gateways ack unconditionally with STAT_OK
     *    there is no common sync point yet at this stage of processing the cmds
     */
    if (!espnow_sol_pkt.len) {          // check buffer free first to avoid mutual overwrites
#if defined(MCOM_ARD)
        len = _min(len, _SZ(espnow_sol_pkt.data) - 1);  // allow for appending a zero char
#else   // if defined(MCOM_ARD)
        len = min(len, _SZ(espnow_sol_pkt.data) - 1);   // allow for appending a zero char
#endif  // if defined(MCOM_ARD)

        memcpy(espnow_sol_pkt.data, data, len);
        espnow_sol_pkt.len = len;
        espnow_sol_pkt.rssi = info->rx_ctrl->rssi;
        xEventGroupSetBits(espnow_sol_events, ESPNOW_EVENT_RESPONSE_RECEIVED(0));
    }
}

#elif defined(ESPNOW_TARGET)

#define ESPNOW_UNSOL_QUE_DEPTH 10

// check for existence of que to allow ESPNOW_QUE_DEINIT() to be a stub
// due to tasks and such can't be deleted easily
#define ESPNOW_QUE_INIT() \
do { \
    if (!espnow_unsol_que) { \
        espnow_unsol_que = xQueueCreate(ESPNOW_UNSOL_QUE_DEPTH, _SZ(espnow_packet_t)); \
        xTaskCreate(espnow_unsol_task, "espnow_unsol_task", 4096, 0, 5, 0); \
    } \
} while (0)

/*
 * a stub for now as it is too complicated
 */
#define ESPNOW_QUE_DEINIT()

/*
 * ESPNOW_TARGETs add peers implicitly -> no ESPNOW_ADD_PEERS(0 needed
 */
#define ESPNOW_ADD_PEERS() 

QueueHandle_t espnow_unsol_que;    // running at _SZ(espnow_packet_t) size

/*
 * commands lost because espnow_unsol_que was full when one arrived.
 *
 * a drop here is INVISIBLE otherwise - it looks exactly like a command that never made it over
 * the air, which is the one thing the multi-antenna setup cannot distinguish on its own. the
 * gateway reports this in @state (see crash_misc_str() in ultra_espnow_gw.c).
 *
 * single producer - the recv callback - so a plain ++ needs no protection.
 */
_u32 espnow_unsol_dropped;

/*
 * this is for unsolicited packets e.g. cmds
 */
void
espnow_recv_cb(const esp_now_recv_info_t *info,
                           const _u8 *data, int len)
{
    espnow_packet_t espnow_unsol_pkt;

#if defined(MCOM_ARD)
    len = _min(len, _SZ(espnow_unsol_pkt.data));
#else   // if defined(MCOM_ARD)
    len = min(len, _SZ(espnow_unsol_pkt.data));
#endif  // if defined(MCOM_ARD)

    memcpy(espnow_unsol_pkt.data, data, len);
    espnow_unsol_pkt.len = len;
    espnow_unsol_pkt.rssi = info->rx_ctrl->rssi;
    memcpy(espnow_unsol_pkt.src_mac, info->src_addr, ESP_NOW_ETH_ALEN);
    memcpy(espnow_unsol_pkt.dst_mac, info->des_addr, ESP_NOW_ETH_ALEN);
#if defined(GW_TIMING_TRACE)
    espnow_unsol_pkt.t_rx = esp_timer_get_time();   // ISR safe, reads the hw timer
#endif
    if (xQueueSendFromISR(espnow_unsol_que, &espnow_unsol_pkt, 0) != pdPASS) {
        ++espnow_unsol_dropped;     // queue full: the command is GONE, and silently so before this
    }
}

#else   // if defined(ESPNOW_INITIATOR)/ elif defined(ESPNOW_TARGET)
#   error this may not happen

#endif

// s_espnow_sta_netif required only if ESPNOW_WIFI_SETUP() and ESPNOW_WIFI_UNSETUP() is used
// AKA if NOT operating in WiFi mode (STA or AP)
// since OTA over WiFi also requires WIFI_INITIATOR to be defined there is no unique criterion to decide -> so use ESP32_() macro
// and enter all devices that (also) operate in true ESPNOW_WIFI_SETUP() mode (AKA w/o ur_connect(), not considering OTA)
// UPDATE as of 2026_06_28: define this per default / only exclude those few required
//#if ESP32_(45) || ESP32_(46) || 
//    ESP32_(55) || ESP32_(57) || ESP32_(61) || ESP32_(65) || 
//    ESP32_(75) || ESP32_(77)
//
// UPDATE as of 2026_08_13: ESPNOW_WIFI_SETUP() no longer creates a netif (it needs none), so this
// is unused and ESPNOW_WIFI_UNSETUP() has nothing left to destroy -> all three flip back on together
//#if !ESP32_(37) && !ESP32_(41) && !ESP32_(44) && !ESP32_(69)     // <= mainly: seed studio XIAO ESP32-S3 8 MB PSRAM/ 8 MB Flash
//static esp_netif_t *s_espnow_sta_netif = 0;
//#endif

#define ESPNOW_ADD_PEER(mac) \
do { \
    /* If MAC address does not exist in peer list, add it */ \
    if (!esp_now_is_peer_exist(mac)) { \
        esp_now_peer_info_t peer = { 0 }; \
        /* peer.channel = 0; */   /* keep channel as is / already setup in ESPNOW_WIFI_SETUP() */ \
        peer.ifidx = WIFI_IF_STA; \
        /* peer.encrypt = false; */ \
        memcpy(peer.peer_addr, (mac), ESP_NOW_ETH_ALEN); \
        ESP_ERROR_CHECK(esp_now_add_peer(&peer)); \
    } \
} while (0)

#define ESPNOW_INIT() \
do { \
    ESPNOW_QUE_INIT(); \
    ESP_ERROR_CHECK(esp_now_init()); \
    ESP_ERROR_CHECK(esp_now_register_recv_cb(espnow_recv_cb)); \
    ESPNOW_ADD_PEERS(); /* ESPNOW_TARGETS only */ \
} while (0)

#define ESPNOW_DEINIT() \
do { \
    ESP_ERROR_CHECK(esp_now_unregister_recv_cb()); \
    ESP_ERROR_CHECK(esp_now_deinit()); \
    ESPNOW_QUE_DEINIT(); \
} while (0)

#define isESPNOW(a) (!*(a))         // effectively tests on a null string

#endif  // defined(ESPNOW_TARGET) || defined(ESPNOW_INITIATOR)

/* ---^^^--- ESPNOW section ---^^^----------------------------------------------------------------------------- */


#if defined(MCOM_ARD)
/* ---vvv--- arduino specific ---vvv--------------------------------------------------------------------------- */
/* ---vvv--- WiFi section (ard runtime) ---vvv----------------------------------------------------------------- */

#if defined(NO_DHCP)

/*
 * see comments to this issues on corresponding IDF part down below
 */
RTC_DATA_ATTR _i8 cached_ip[16];        // assigned per IPSTR == 16 chars
RTC_DATA_ATTR _i8 cached_gateway[16];
RTC_DATA_ATTR _i8 cached_subnet[16];
RTC_DATA_ATTR _i8 cached_dns[16];
#endif  // NO_DHCP

#define GENERAL_RETRY_TIMEOUT    6000   // don't try most cmds longer than this/ see comments [ EXPERIMENTAL ONLY! ]
#define WLAN_RECONNECT_TIMEOUT  10000   // try to actively reconnect WLAN after this as built-in c... does not work?!
#define ACCESSPT_CONN_RETRY_DELAY   1   // retry rate to connect to access point

// WARNING: This function is called from a separate FreeRTOS task (thread)!
void 
WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) 
{
//TP05
    PR02("WiFi Event [ %lu ] %s\n", tstamp(), give_wifi_event(event));
    switch (event) {
      case ARDUINO_EVENT_WIFI_READY:               break;
      case ARDUINO_EVENT_WIFI_SCAN_DONE:           break;
      case ARDUINO_EVENT_WIFI_STA_START:           break;
      case ARDUINO_EVENT_WIFI_STA_STOP:            break;
      case ARDUINO_EVENT_WIFI_STA_CONNECTED:       break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:  
#if DEBUG > 5
        PR00("disc reason: %s\n", give_disc_reason(info.wifi_sta_disconnected.reason));
        WIFI_DIAG(0);
#endif
        break;
      case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE: break;
      case ARDUINO_EVENT_WIFI_STA_GOT_IP:          
#if DEBUG > 5
        PR00("NEW IP address: " IPSTR "\n", IP2STR(&info.got_ip.ip_info.ip));
        WIFI_DIAG(1);
#endif

#if defined(NO_DHCP)
        if (*cached_ip) {       
            PR02("received previously set static IP data in WiFiEvent() -> doing nothing\n");
        } else {
            // cache DHCP data if: not already done or cache marked invalid
            sprintf(cached_ip,      WiFi.localIP().toString().c_str());
            sprintf(cached_gateway, WiFi.gatewayIP().toString().c_str());
            sprintf(cached_subnet,  WiFi.subnetMask().toString().c_str());
            sprintf(cached_dns,     WiFi.dnsIP(0).toString().c_str());
            PR02("received DHCP data in WiFiEvent() -> caching\n");
        }
#if DEBUG > 1
        GS05(cached_ip);
        GS05(cached_gateway);
        GS05(cached_subnet);
        GS05(cached_dns);
#endif  
#endif  // NO_DHCP

        break;
      case ARDUINO_EVENT_WIFI_STA_LOST_IP:         break;
      case ARDUINO_EVENT_WPS_ER_SUCCESS:           break;
      case ARDUINO_EVENT_WPS_ER_FAILED:            break;
      case ARDUINO_EVENT_WPS_ER_TIMEOUT:           break;
      case ARDUINO_EVENT_WPS_ER_PIN:               break;
      case ARDUINO_EVENT_WIFI_AP_START:            break;
      case ARDUINO_EVENT_WIFI_AP_STOP:             break;
      case ARDUINO_EVENT_WIFI_AP_STACONNECTED:     break;
      case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:  break;
      case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:    break;
      case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:   break;
      case ARDUINO_EVENT_WIFI_AP_GOT_IP6:          break;
      case ARDUINO_EVENT_WIFI_STA_GOT_IP6:         break;
      case ARDUINO_EVENT_ETH_GOT_IP6:              break;
      case ARDUINO_EVENT_ETH_START:                break;
      case ARDUINO_EVENT_ETH_STOP:                 break;
      case ARDUINO_EVENT_ETH_CONNECTED:            break;
      case ARDUINO_EVENT_ETH_DISCONNECTED:         break;
      case ARDUINO_EVENT_ETH_GOT_IP:               break;
      default:                                     break;
    }
}

_i32
wait4wifi()
{
TP05
    _u32 last;
    _i32 stat = 0;
    _i8 issue_cr = 0;
    _i32 WiFi_status;

    PR02("waiting for WiFi: ");
    PR02("\n<TPx: %lu>\n", tstamp()); 
    last = tstamp();                
    while ((WiFi_status = WiFi.status()) != WL_CONNECTED) {
        if (GENERAL_RETRY_TIMEOUT < tstamp() - last) {
            PR00("\nno WiFi connection: 0x%0x [ %s ]\n", WiFi_status, give_wifi_status(WiFi_status));
            stat = 1;
            goto end;
        }
        printf("#");                          // <== timing! keep this independent from DEBUG
        issue_cr = 1;
        vTaskDelay(pdMS_TO_TICKS(ACCESSPT_CONN_RETRY_DELAY));
    }
    if (issue_cr) printf("\n");            // <== timing! keep this independent from DEBUG
    PR02("is up\n");
    PR02("\n<TPx: %lu>\n", tstamp());

    /*
     * mega hack:
     * as we know that:
     *  - at the time of wait4wifi() function init_3rd() has already passed ssid_last is already setup properly
     *  - ACCESSPOINT_CONNECT_BOOSTER relevant for host6 (== TETHER_SSID) even if DHCP disabled
     *  - disable for all others to save time
     */
//#define ACCESSPOINT_CONNECT_BOOSTER_DELAY 1             // stat: 1101
//#define ACCESSPOINT_CONNECT_BOOSTER_DELAY 2             // stat: 162      sustained(!)
//#define ACCESSPOINT_CONNECT_BOOSTER_DELAY 3             // stat: 164      sustained(!)
//#define ACCESSPOINT_CONNECT_BOOSTER_DELAY 4             // stat: 164      sustained(!)
//#define ACCESSPOINT_CONNECT_BOOSTER_DELAY 5             // stat: 167
#define ACCESSPOINT_CONNECT_BOOSTER_DELAY 10            // stat: 169
//#define ACCESSPOINT_CONNECT_BOOSTER_DELAY 50            // stat: 212
//#define ACCESSPOINT_CONNECT_BOOSTER_DELAY 100            // stat: 264
//#define ACCESSPOINT_CONNECT_BOOSTER_DELAY 150            // stat: 313     some exceptions
//#define ACCESSPOINT_CONNECT_BOOSTER_DELAY 200            // stat: 

#if defined(ACCESSPOINT_CONNECT_BOOSTER)
    extern _u8 ssid_last;
    if (ssid_last == TETHER_SSID) {
        PR02("ACCESSPOINT_CONNECT_BOOSTER_DELAY configured to: %d\n", ACCESSPOINT_CONNECT_BOOSTER_DELAY);
        vTaskDelay(pdMS_TO_TICKS(ACCESSPOINT_CONNECT_BOOSTER_DELAY));
    } else {
        PR02("ACCESSPOINT_CONNECT_BOOSTER_DELAY not active for != TETHER_SSID\n");
    }
#endif

end:
    return stat;
}

// check if WLAN still is there. WHY IS THAT REQUIRED?!
_i32
myconn_check() 
{
//TP05          // a little too much if in main loop and DEBUG == 10 
    _u32 cur = tstamp();
    static _u32 last;       // inited to 0 -> implies max. TIMEOUT of WLAN_RECONNECT_TIMEOUT after reboot
    static _u8 connected;

    if (WiFi.status() == WL_CONNECTED) {
        if (!connected && last) {
            PR01("WLAN (re)connected\n");
        }
        connected = 1;
        last = cur;
    } else {
        if (cur - last < WLAN_RECONNECT_TIMEOUT) {
#if DEBUG > 5
            PR00(">");
            vTaskDelay(pdMS_TO_TICKS(200));
#endif
        } else {
            PR01("\nWLAN lost, trying to reconnect\n");
            WiFi.disconnect();
            WiFi.reconnect();
            last = cur;
        }
        connected = 0;
    }
    return !connected;
}
/* ---^^^--- WiFi section (ard runtime) ---^^^----------------------------------------------------------------- */


/* ---vvv--- cmd to status (ard mysend) ---vvv----------------------------------------------------------------- */
#if defined(WIFI_INITIATOR) || defined(ESPNOW_INITIATOR) || defined(ETH_INITIATOR)

#if defined(WIFI_INITIATOR)

WiFiClient target;

#define TARGET_CONNECT_RETRY_DELAY  1   // retry rate to connect target
#define TARGET_STATUS_RETRY_DELAY   1   // retry rate to retrieve status
#define TARGET_MYSEND_RETRY_DELAY   250 // retry rate to retry mysend() as a whole

//
// in case the TARGET_HOST in use can't provide a retry feature itself we
// attempt to apply 'some' fail save alike thingy ourselves
//
#define RETRY(_cmd, _retries) \
{ \
    _u32 _retry = (_retries); \
    while (_cmd & 0b0010 && _retry--) vTaskDelay(pdMS_TO_TICKS(TARGET_MYSEND_RETRY_DELAY)); \
}

#endif  // if defined(WIFI_INITIATOR)

/*
 *    condition:                    statmsg:  stat: binary stat:
 *
 *    status ok                     STAT_STAT 0     0000
 *    no WiFi conn (so no status)   "err#1"   1     0001
 *    no target conn (so no status) "err#2"   2     0010    eligible for retry
 *    conn but no status            "err#3"   3     0011    eligible for retry
 *    wrong status format           "err#4"   4     0100
 *    status nok                    "err#5"   5     0101
 */
_i32
mysend(_i8cp cmd, _i8cp host, _u16 port, _i8cp *statmsg)
{
TP05

#if defined(WIFI_INITIATOR) || defined(ETH_INITIATOR)

    _u32 last, now;
    _i32 stat;
    _i32 err;
    String line = "";
    _i8 _host[64];      // c_str() temporary f...

    PR01("cmd: %lu [%s] -> [[%s]:%u]\n", tstamp(), cmd, host, port);
    if (wait4wifi()) {
        stat = 1;
        goto end;
    }
    PR01("TP02: %lu WiFi %s\n", tstamp(), "connected");
    // WiFi.gatewayIP().toString().c_str() is known NOT BEFORE connected!
    if (!strcmp(host, KARR_TARGET_HOST)) {

        // a simple assignment is not possible in c++ f...... s...?!?!
        //host = (_i8p)WiFi.gatewayIP().toString().c_str(); // replace placeholder by IP

        // instead inefficiently copy like this:
        *_host = 0;
        strcat(_host, WiFi.gatewayIP().toString().c_str()); // consider the implicit 0 always added to dst

        // now you assign as always
        host = _host;
    }
    last = now = tstamp();                
    do {
        /*
         * trick to autoconvert host to a valid address since we don't have it prior to this
         * -> mostly needed to remote-control a tethered smartphone via WiFi
         */
        if (target.connect(host, port)) {
            break;
        }
        printf("|");                          // <== timing! keep this independent from DEBUG
        vTaskDelay(pdMS_TO_TICKS(TARGET_CONNECT_RETRY_DELAY));
        now = tstamp();
    } while (GENERAL_RETRY_TIMEOUT > now - last);
    if (!(GENERAL_RETRY_TIMEOUT > now - last)) {
        PR00("\nno target connection\n");
        stat = 2; 

#if defined(NO_DHCP)
        /*
         * no FIXME alert on Arduino ATM:
         * timeout comes fast in any case (see mysend() of IDF for comparison)
         *
         * typically we reach this code path if static IP stuff
         * makes wrong assumptions. so as a first resort we try to 
         * temporarily reanimate dynamic IP code (aka DHCP)
         */
        *cached_ip = 0;
        beep(BEEP_ERR, 1);      // signal cache invalidation 
        PR02("clearing cached DHCP data due to target.connect() fail\n");
#endif  // NO_DHCP

        goto end;
    }
    if (last != now) printf("\n");            // <== timing! keep this independent from DEBUG
//    PR02("cmd: %s -> [%s:%u]\n", cmd, host, port);
    if (target.printf("%s\n", cmd) != strlen(cmd) + 1) {
        PR00("could not send all bytes\n");
    }
    last = now = tstamp();
    do {
        if (target.available()) {
            line = target.readStringUntil('\n');
            break;
        }
        printf(".");                          // <== timing! keep this independent from DEBUG
        vTaskDelay(pdMS_TO_TICKS(TARGET_STATUS_RETRY_DELAY)); 
        now = tstamp();
    } while (GENERAL_RETRY_TIMEOUT > now - last);
    printf("\n");                             // <== timing! keep this independent from DEBUG
    if (!(GENERAL_RETRY_TIMEOUT > now - last) || !line.length()) {
        PR00("no status received (yet)\n");
        stat = 3;
    } else {
        PR01("stat: %lu %s\n", tstamp(), line.c_str());
        if (err = regexec(&_regex, line.c_str(), _NE(_pmatch), _pmatch, 0)) {
            _i8 _buf[128];

            // no match
            regerror(err, &_regex, _buf, _SZ(_buf));
            PR02("%s\n", _buf);
            stat = 4;
        } else if (strncmp("0", line.c_str() + _pmatch[STAT_STAT].rm_so, 1)) {
            PR02("status failed\n");
            stat = 5;
        } else {
            // valid res
            if (statmsg) {
                static _i8 _buf[128];

                *_buf = 0;
                strncat(_buf, line.c_str() + _pmatch[STAT_MISC].rm_so, _pmatch[STAT_MISC].rm_eo - _pmatch[STAT_MISC].rm_so);
                *statmsg = _buf;                                      
            }
            stat = 0;
        }
    }
end:
    if (stat) {
        if (statmsg) *statmsg = _err[stat];
        beep(BEEP_ERR, 3);
    } else {
        beep(BEEP_OK, 1);
    }
    return stat;


#else // if defined(WIFI_INITIATOR) || defined(ETH_INITIATOR)


    _i32 stat, err;

    /*
     * code path for ESPNOW only
     *
     * e.g.
     * ESPNOWcmd: 145 [@beep= f:1000 c:1 t:.05 p:.25 g:-20 ^host2 ^] len 43 -> espnow:0
     * i.e. NO trailing '\n'
     */
    PR01("espnow cmd: %lu [%s] len %d -> gateway\n", tstamp(), cmd, strlen(cmd));
    espnow_sol_pkt.len = 0; // indicate buffer free for cb routines
    xEventGroupClearBits(espnow_sol_events, ESPNOW_EVENT_RESPONSE_MASK);

    /*
     * cmd has no trailing '\n" but is sent inclusive the terminating null / also reflected in pkt.len
     */
    for (_u32 i = 0; i < NUM_ESPNOW_GATEWAYS; ++i) {
        ESP_ERROR_CHECK(esp_now_send(espnow_gateway_mac[i], (_u8p)cmd, strlen(cmd) + 1)); // copy terminating 0
        PR02("esp_now_send %d " MACSTR "\n", i, MAC2STR(espnow_gateway_mac[i]));
    }
    EventBits_t bits = xEventGroupWaitBits(
        espnow_sol_events,
        ESPNOW_EVENT_RESPONSE_MASK,
        pdTRUE,
        pdFALSE,
        pdMS_TO_TICKS(ESPNOW_STATUS_TIMEOUT)
    );
    /*
     * the status received contains a trailing '\n' and is sent without terminating null
     * to match historic mysend behavior
     */
    if (bits & ESPNOW_EVENT_RESPONSE_MASK) {
        // any of the possible RCV packets alone is sufficient to cancel the timeout of all
        // care must be taken to toss duplicates right before coming here:
        //  for pre-acked cmds tcp_server does the hard work and espnow_sol_pkt.len check prevents from mutual overwrite
        //  for non-pre-acked cmds the target application must handle this
    } else {
        PR01("espnow stat: timeout\n");
    }

    /*
     * common code path for INITIATORS ETH, WiFi, ESPNOW
     *
     * the status received contains a trailing '\n' and is NOT null terminated
     * to match historic mysend behavior
     */
    if (espnow_sol_pkt.len > 0 && espnow_sol_pkt.data[espnow_sol_pkt.len - 1] == '\n') {
        espnow_sol_pkt.data[espnow_sol_pkt.len - 1] = 0;
        // f...... stuff must be casted explicitly since [ _i8 != (signed char) ] for this i....
        PR01("stat: %lu %s, rssi %d\n", tstamp(), espnow_sol_pkt.data, (signed char)espnow_sol_pkt.rssi); 
        if (err = regexec(&_regex, (_i8p)espnow_sol_pkt.data, _NE(_pmatch), _pmatch, 0)) {
            _i8 _buf[128];

            // no match
            regerror(err, &_regex, _buf, _SZ(_buf));
            PR02("%s\n", _buf);
            stat = 4;
        } else if (strncmp("0", (_i8p)espnow_sol_pkt.data + _pmatch[STAT_STAT].rm_so, 1)) {
            PR02("status failed\n");
            stat = 5;
        } else {
            // valid res
            if (statmsg) {
                static _i8 _buf[128];

                *_buf = 0;
                strncat(_buf, (_i8p)espnow_sol_pkt.data + _pmatch[STAT_MISC].rm_so, _pmatch[STAT_MISC].rm_eo - _pmatch[STAT_MISC].rm_so);
                *statmsg = _buf;                                      
            }
            stat = 0;
        }
    } else {
        PR00("invalid read count and/or unterminated data\n");
        stat = 3;
    }
//ORIG from socket out1:
//ORIG from socket    if (s >= 0) close(s);
    if (stat) {
        if (statmsg) *statmsg = _err[stat];
        beep(BEEP_ERR, 3);
        PR02("statmsg: %s\n", _err[stat]);
    } else {
        beep(BEEP_OK, 1);
    }
    return stat;

#endif // if defined(WIFI_INITIATOR) || defined(ETH_INITIATOR)
}
#endif  // if defined(WIFI_INITIATOR) || defined(ESPNOW_INITIATOR) || defined(ETH_INITIATOR)
/* ---^^^--- cmd to status (ard mysend) ---^^^----------------------------------------------------------------- */


/* ---vvv--- service control port (ard myserv) ---vvv---------------------------------------------------------- */
#if defined(MYSERVICE_PORT)

WiFiClient myclient;
WiFiServer myservice(MYSERVICE_PORT);

/*
 * as of bin/p 2022_10_24
 *
 * func server_status(cmd, stat, misc \
 *    )
 * {
 *    misc = misc ? misc : 0
 *    stat = stat ? stat : 0
 *    if (stat) beep(BEEP_ERR, 3)
 *    if (cmd != M["player"] A["init"]) {
 *        PR(strftime("%T: ") "-----------[ " cmd " - " (stat ? "NOK" : "OK ") "]------------")
 *    }
 *    return "#["     cmd      "]" \
 *           "#[" !!IS_REPEAT  "]" \
 *           "#[" !!IS_STEALTH "]" \
 *           "#["   PLAY_MODE  "]" \
 *           "#["     misc     "]" \         # <- retval for variables
 *           "#["     stat     "]"
 * }
 *
 *
 * 17570 toh@host2[/home/toh] > echo @OTA | nc esp32-2 8888
 * #[ar]#[0]#[0]#[0]#[-78]#[0]
 *
 */
String
statusStr(_i8p cmd, _i8 stat, _i8p misc)
{
    String str;
    str.reserve(1024);

    str = "#[";
    str += cmd;     // cmd
    str += "]#[";
    str += "0";     // IS_REPEAT
    str += "]#[";
    str += "0";     // IS_STEALTH
    str += "]#[";
    str += "0";     // PLAY_MODE
    str += "]#[";
    str += misc;
    str += "]#[";
    str += stat ? "1" : "0";
    str += "]";
    return str;
}
#endif
/* ---^^^--- service control port (ard myserv) ---^^^---------------------------------------------------------- */


/* ---vvv--- initialization (ard variant) ---vvv--------------------------------------------------------------- */
RTC_DATA_ATTR _u8  ssid_last = 0;
RTC_DATA_ATTR _u32 bootCount = 0;

_i32
init_1st() 
{
TP05
    ++bootCount;
    return 0;
}

_i32
init_2nd() 
{
TP05
    PR02("bootCount: %u\n", bootCount);
#if DEBUG > 5
    PR00("esp-idf: %d.%d.%d\n", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
#if defined(ESP_ARDUINO_VERSION_MAJOR)
    PR00("ard-esp32: %d.%d.%d\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
#endif
    PR00("DEBUG: %d\n", DEBUG);
    PR00("HOST: %s\n", HOST);
    PR00("FW: %s\n", DEVICE_FW);

    const esp_app_desc_t *app_desc_ptr;
#if ESP_ARDUINO_VERSION > ESP_ARDUINO_VERSION_VAL(2,0,17)       // aka since 3.0.4
    app_desc_ptr = esp_app_get_description();
#else
    app_desc_ptr = esp_ota_get_app_description();
#endif
    GS05(app_desc_ptr->project_name);
    GS05(app_desc_ptr->time);
    GS05(app_desc_ptr->date);
    GS05(app_desc_ptr->idf_ver);

    print_reset_reason(0);
    print_reset_reason(1);
    print_wakeup_reason();
#endif      // if DEBUG > 5
#if DEBUG > 1

    /*
     * // type
     * #define PART_TYPE_APP 0x00
     * 
     * // sub type
     * #define PART_SUBTYPE_FACTORY  0x00
     * #define PART_SUBTYPE_OTA_FLAG 0x10
     * #define PART_SUBTYPE_OTA_MASK 0x0f
     * #define PART_SUBTYPE_TEST     0x20
     * 
     * // ota state
     * ESP_OTA_IMG_NEW             = 0x0U,         Monitor the first boot. In bootloader this state is changed to ESP_OTA_IMG_PENDING_VERIFY. 
     * ESP_OTA_IMG_PENDING_VERIFY  = 0x1U,         First boot for this app was. If while the second boot this state is then it will be changed to ABORTED. 
     * ESP_OTA_IMG_VALID           = 0x2U,         App was confirmed as workable. App can boot and work without limits. 
     * ESP_OTA_IMG_INVALID         = 0x3U,         App was confirmed as non-workable. This app will not selected to boot at all. 
     * ESP_OTA_IMG_ABORTED         = 0x4U,         App could not confirm the workable or non-workable. In bootloader IMG_PENDING_VERIFY state will be changed to IMG_ABORTED. This app will not selected to boot at all. 
     * ESP_OTA_IMG_UNDEFINED       = 0xFFFFFFFFU,  Undefined. App can boot and work without limits. 
     */

    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    esp_ota_img_states_t ota_state;

    if (running != configured) {
        PR00("==================== running and configured different ====================\n");
    }
#if ESP_ARDUINO_VERSION > ESP_ARDUINO_VERSION_VAL(2,0,17)       // aka since 3.0.4
    PR06("actual OTA partition type %d subtype %d offset 0x%08lx ", running->type, running->subtype, running->address);
#else
    PR06("actual OTA partition type %d subtype %d offset 0x%08lx ", running->type, running->subtype, (long unsigned int)running->address);
#endif
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        PR00("ota_state: 0x%x\n", ota_state);
    } else {
        PR00("ota_state: NOT AVAIL\n");
    }
#if 0
    if (running->address == 0x00210000) {
        esp_ota_mark_app_invalid_rollback_and_reboot();
        // esp_ota_mark_app_valid_cancel_rollback();
    }
#endif
#endif      // if DEBUG > 1
#if defined(WIFI_INITIATOR) || defined(ESPNOW_INITIATOR) || defined(ETH_INITIATOR)
    _i32 err;
    if (err = regcomp(&_regex, STATUS_MATCH, REG_EXTENDED)) {
        _i8 _buf[128];

        regerror(err, &_regex, _buf, _SZ(_buf));
        PR00("%s\n", _buf);
    }
#endif  // if defined(WIFI_INITIATOR) || defined(ESPNOW_INITIATOR) || defined(ETH_INITIATOR)
#if defined(BUZZER)
    beep_init();
    if (bootCount == 1) {
        beep(BEEP_INFO, 2);
    }
#endif
#if defined(VBAT_ADC1_SENSE_PIN)
    vbat_monitor_init();
#endif
    return 0;
}

_i32
init_3rd(_u8 ssid, _i8cp prg) 
{
TP05
    WiFi.onEvent(WiFiEvent);    // register all WiFi events

    /*
     * once programmed -> purge it even if given explicitly
     *
     * COMMENT #1:
     * UPDATE as of 2023_01_10:
     *  - always set WiFi.persistent(false);  and
     *  - always set WiFi.begin(ssid, pass);
     * as our ssid is always hardcoded in program (no dynamic change required)
     * so no need to save dynamic things to NVRAM arises
     *
     * as a result we now always sequence through:
     *  WiFi.persistent(false);
     *  WiFi.begin(ssid, pass);
     *
     * the previous version would not reprogram ssid after boot 1
     * which may conflict with some newer use cases (multi AP remotes)
     *
     * you must initially issue an valid SSID and PASS.
     * this then will be saved to non volatile RAM.
     * after this feel free to set this to 0 to speed
     * up the setup process and to avoid wear.
     *
     * i.e.
     * avoid WiFi.persistent() setting to true
     */
    PR02("AP requested: %s\n", accpts[(ssid << 1) + 0]);
    if (ssid_last != ssid) {  // see COMMENT #1
        PR02("SSID: EXPLICIT, SAVING TO NVRAM\n");
        WiFi.persistent(true);  // save params in NVRAM when new connect with new SSID params is given

#if defined(NO_DHCP)
        // accesspoint changes imply DHCP cache invalidation
        *cached_ip = 0;
        beep(BEEP_ERR, 1);      // signal cache invalidation 
        PR02("clearing cached DHCP data due to SSID change\n");
#endif  // NO_DHCP

    } else {
        PR02("SSID: NOT EXPLICIT, REUSING NVRAM\n");
        WiFi.persistent(false); // avoid wear through saving data to mem // Don't save WiFi configuration in flash
    }
    WiFi.mode(WIFI_STA);

#if 0   // VALID FOR ESP32_(37)
    //
    // VALID FOR ESP32_(37)
    //

    /*
     * huge hack TAKE CARE:
     *
     ** WIFI Issues with ESP32-S3-WROOM-1 modules that don't have a CMIIT ID - ESP32 Forum
     * https://www.esp32.com/viewtopic.php?t=41899&p=137765#p137764
     * 
     * On some -C3 "Supermini" boards I found that I had to 
     * actually reduce the WiFi TX power from the default (20dBm?) 
     * (used esp_wifi_set_max_tx_power()) to be able to connect 
     * to a network. Probably due to a "sub-optimal" matching 
     * network and/or antenna on the board. IIRC, I could only 
     * go up to 11 or 13dBm before connection issues appeared. 
     * May be worth trying out.
     */

    /*
     * 5. Relationship between set value and actual value.  As follows: 
     * {set value range, actual value} = 
     * {{[8, 19],8}, 
     * {[20, 27],20}, 
     * {[28, 33],28}, 
     * {[34, 43],34}, 
     * {[44, 51],44}, 
     * {[52, 55],52}, 
     * {[56, 59],56}, 
     * {[60, 65],60}, 
     * {[66, 71],66}, 
     * {[72, 79],72}, 
     * {[80, 84],80}}.
     */

    //ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(80)); // works NOT
      ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(72)); // works TOP
                                                          //stat: 180 res: OK TEST_CASE RPIZ all in a row!!
                                                          //stat: 184 res: OK   
                                                          //stat: 174 res: OK
                                                          //stat: 181 res: OK
                                                          //stat: 176 res: OK
                                                          //stat: 177 res: OK
                                                          //stat: 186 res: OK
                                                          //stat: 177 res: OK
                                                          //stat: 177 res: OK
                                                          //stat: 185 res: OK
                                                          //stat: 178 res: OK
                                                          //stat: 186 res: OK  
    //ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(66)); // works
    //ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(60)); // works
    //ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(56)); // 
    //ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(52)); // 
    //ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(44)); // 
    //ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(34)); // works 
    //ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(28)); // works goodly (best so far)
    //ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(20)); // works NOT
    //ESP_ERROR_CHECK(esp_wifi_set_max_tx_power(8)); // works NOT
#endif  // VALID FOR ESP32_(37)

    REDUCE_WIFI_POWER_IF_REQUIRED();

#if defined(NO_DHCP)
    if (*cached_ip) {
        PR02("applying cached DHCP data in init_3rd() -> setting static IP\n");
#if ESP_ARDUINO_VERSION > ESP_ARDUINO_VERSION_VAL(2,0,17)       // aka since 3.0.4
        if (!WiFi.config(                   cached_ip,
                                            cached_gateway,
                                            cached_subnet, 
                                            cached_dns, 
                                            cached_dns)) {
#else
        // /home/toh/esp.v4.4.7-ard.2.0.17/esp-idf.v4.4.7 needs the f...... (const uint8_t*) cast
        if (!WiFi.config(   (const _u8*)cached_ip,
                            (const _u8*)cached_gateway,
                            (const _u8*)cached_subnet, 
                            (const _u8*)cached_dns, 
                            (const _u8*)cached_dns)) {
#endif
            PR00("static IP WiFi configuration failed\n");
            beep(BEEP_ERR, 3);
            return 1;
        } /*}*/
    } else {
        PR02("no cached DHCP data avail in init_3rd() -> expecting lease\n");
    }
#endif  // NO_DHCP

/*
defaults (NVRAM MUST BE CLEARED EXPLICITLY FOR THIS!):

val: country.cc[]: <01 >
val: w_country.cc[]: <01 >
val: w_country.schan == 0x1 1
val: w_country.nchan == 0xb 11
val: w_country.max_tx_power == 0x14 20
val: w_country.policy == 0x0 0

*/
#if 0
    // Define regulatory country: Germany
    wifi_country_t country = {
        .cc = "DE",         // 2-letter country code
        .schan = 1,         // Start channel
        .nchan = 13,        // Number of channels
//        .max_tx_power = 20, // Max TX power (in dBm)  // not required since ignored anyway
        .policy = WIFI_COUNTRY_POLICY_MANUAL
    };
    ESP_ERROR_CHECK(esp_wifi_set_country(&country));    // Apply before WiFi.begin()
#elif 0
    wifi_country_t country = {
        .cc = "US",
        .schan = 0x1,
        .nchan = 0xb,
        .max_tx_power = 0x1e,     // ignored
        .policy = WIFI_COUNTRY_POLICY_MANUAL,
    };
    ESP_ERROR_CHECK(esp_wifi_set_country(&country));
#elif 0
    ESP_ERROR_CHECK(esp_wifi_set_country_code("US", 0));    // automatically sets: w_country.policy == 0x1
#endif

    if (ssid_last != ssid) { 
        PR02("set ssid_last from %x -> %x\n", ssid_last, ssid);
        ssid_last = ssid;
        WiFi.begin(accpts[(ssid << 1) + 0], accpts[(ssid << 1) + 1]); 
    } else {
        WiFi.begin(); 
    }
    PR02("TP01: %lu WiFi %s\n", tstamp(), "issued");
//an aggregated line length (even if split across several lines) exceeding about 166 chars produces a massive time loss
    PR06("|<----[ MAXIMUM LINE LENGTH ACCEPTABLE I.E. BELOW 10ms ]-------------------------------------------------------->|\n");
    PR02("[ %s on %s ] ready to operate, WiFi stat: %s\n", prg, HOST, give_wifi_status(WiFi.status()));

//must be called after WiFi.begin()
#if defined(DUMP_SOME)
dump_some(WIFI_MODE_STA, __func__);
#endif

    return 0;
}
/* ---^^^--- initialization (ard variant) ---^^^--------------------------------------------------------------- */


/* ---vvv--- ESPNOW init_3rd() replacement initialization (ard variant) ---vvv--------------------------------- */
/*
 * replacement for init_3rd() if full blown WiFi is not in use
 */
#define ESPNOW_WIFI_SETUP() \
    WiFi.mode(WIFI_STA); \
    WiFi.setChannel(ESPNOW_CHANNEL);
/* ---^^^--- ESPNOW init_3rd() replacement initialization (ard variant) ---^^^--------------------------------- */
/* ---^^^--- arduino specific ---^^^--------------------------------------------------------------------------- */


#else   // defined(MCOM_ARD)

//
// for OTA over WiFi to work WIFI_INITIATOR must be defined. 
// this is the case for all devices except ETH-only devices (as WAVESHARE-ETH-P4)
// for these to allow OTA over ETH: #define FW_UPGRADE_VIA_ETH 
//

/* ---vvv--- idf specific ---vvv------------------------------------------------------------------------------- */
/* ---vvv--- WiFi section (idf runtime) ---vvv----------------------------------------------------------------- */
#if defined(WIFI_INITIATOR) || defined(WIFI_TARGET) 
static _u32 wifi_conn_retry = 0;
static _u32 wifi_conn_retry_max = 0;
static esp_netif_t *s_ur_sta_netif = 0;
static SemaphoreHandle_t s_semph_get_ip_addrs = 0;
static _i8 gw_ip[16];
RTC_DATA_ATTR _u8 ssid_last = 0;    // must remeber this across reboots to determine DHCP cache flush

#if defined(NO_DHCP)

/*
IP address strings:

// literally the same:
PR05("NEW IP address: %s\n",      IPAddress(info.got_ip.ip_info.ip.addr).toString().c_str());
// and:
PR05("NEW IP address: " IPSTR "\n", IP2STR(&info.got_ip.ip_info.ip));

#define IPSTR "%d.%d.%d.%d" 123.567.901.345_  16 chars (incl. NULL char) in ASCII repres.
#define IPADDR_NONE         ((u32_t)0xffffffffUL)
#define IP2STR(ipaddr) esp_ip4_addr1_16(ipaddr), \
    esp_ip4_addr2_16(ipaddr), \
    esp_ip4_addr3_16(ipaddr), \
    esp_ip4_addr4_16(ipaddr)

u32_t ipaddr_addr(const char *cp) {     @param cp IP address in ascii representation (e.g. "127.0.0.1")
    return ...;                         @return ip address in network order
}

IP address native 32bit data:

PR05("ip.ip: %lx\n", event->ip_info.ip.addr);   // ip.ip 192.0.2.12 == ip.ip 0x0c0200c0
                                                // 192.0.2.0/24 is the documentation range on purpose: a real
                                                // address would leak into the exports through the packed form,
                                                // which exportit cannot see, and a 192.168.x one would join
                                                // its placeholder list and renumber every other address
*/

RTC_DATA_ATTR _i8 cached_ip[16];        // assigned per IPSTR == 16 chars
RTC_DATA_ATTR _i8 cached_gateway[16];
RTC_DATA_ATTR _i8 cached_subnet[16];
RTC_DATA_ATTR _i8 cached_dns[16];
#endif  // NO_DHCP

#if defined(CACHE_CHANNEL)
/*
 * moved up here from just above ur_wifi_connect(): the fill now lives in
 * ur_handler_on_wifi_connect(), which is defined earlier than that
 */
RTC_DATA_ATTR _i32 cached_channel = 0;
RTC_DATA_ATTR _u8 cached_bssid[6] = {0};
#endif

#define WIFI_SCAN_RSSI_THRESHOLD -127   // default
//#define WIFI_SCAN_RSSI_THRESHOLD -80    // does not connect to RSSI worse than this, use this to force roaming 
#define WIFI_SCAN_METHOD WIFI_FAST_SCAN
#define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK    // minimum mode supported
#define WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL

#define NETIF_DESC_STA "ur"

bool
ur_is_our_netif(const _i8 *prefix, esp_netif_t *netif)
{
TP05
    return !strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1);
}

static void
ur_handler_on_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               __i32 event_id, void *event_data)
{
TP05
    /*
     * the reason code IS the story on an association failure, and it was never printed. these
     * fire on ~2 of 24 wakeups against the BPI-R3 and cost 0.6..3.0s of w_link_up
     */
    {
        wifi_event_sta_disconnected_t *ev = (wifi_event_sta_disconnected_t *)event_data;

        PR01("WiFi disconnect: reason %d [ %s ] rssi %d at %lu\n",
            ev->reason, give_disc_reason(ev->reason), ev->rssi, tstamp());
    }
    wifi_conn_retry++;
    if (wifi_conn_retry > wifi_conn_retry_max) {
        PR00("WiFi Connect failed %d times, stop reconnect.\n", wifi_conn_retry);
        sprintf(gw_ip, "0"); // "0" is special 'IP' indicating error for: permanently unavail
        if (s_semph_get_ip_addrs) {
            xSemaphoreGive(s_semph_get_ip_addrs);
        }
        return;
    }
    PR00("Wi-Fi disconnected, trying to reconnect (%d/%u)...\n", wifi_conn_retry, wifi_conn_retry_max);
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED) {
        return;
    }
    ESP_ERROR_CHECK(err);
}

/*
 * massive timing bug in
 *  [ esp-idf/components/nvs_flash/src/nvs_api.cpp ]
 * in
 *  [ nvs_open_from_partition() ]
 * 
 * according to the following patch:
 * 
 */
#if __WIFI_ALLOW_GRACE_PATCH__   //----------------------------------------------------------------------------------------

diff --git a/components/nvs_flash/src/nvs_api.cpp b/components/nvs_flash/src/nvs_api.cpp
index effed329dd..cbfaef2757 100644
--- a/components/nvs_flash/src/nvs_api.cpp
+++ b/components/nvs_flash/src/nvs_api.cpp
@@ -17,6 +17,7 @@
 #include "esp_err.h"
 #include <esp_rom_crc.h>
 #include "nvs_internal.h"
+#include "/home/toh/bin/mlcf.h"
 
 // Uncomment this line to force output from this module
 // #define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
@@ -262,6 +263,8 @@ static esp_err_t nvs_find_ns_handle(nvs_handle_t c_handle, NVSHandleSimple** han
     return ESP_OK;
 }
 
+RTC_DATA_ATTR _u32 wifi_delay;
+
 extern "C" esp_err_t nvs_open_from_partition(const char *part_name, const char* namespace_name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle)
 {
     esp_err_t lock_result = Lock::init();
@@ -271,6 +274,12 @@ extern "C" esp_err_t nvs_open_from_partition(const char *part_name, const char*
     Lock lock;
     ESP_LOGD(TAG, "%s %s %d", __func__, namespace_name, open_mode);
 
+//PR05("%lu: %s %s %s %d\n", esp_log_timestamp(), __func__, part_name, namespace_name, open_mode);
+if (wifi_delay && !strcmp(namespace_name, "dhcp_state") && open_mode == 0) {
+    PR05("wifi_delay ACTIVE: %dms\n", wifi_delay);
+    usleep(wifi_delay * 1000);
+}
+
     NVSHandleSimple *handle;
     esp_err_t result = NVSPartitionManager::get_instance()->open_handle(part_name, namespace_name, open_mode, &handle);
     if (result == ESP_OK) {

#endif  // __WIFI_ALLOW_GRACE_PATCH__   //----------------------------------------------------------------------------------------

#if defined(WIFI_ALLOW_GRACE)

#if defined(NO_DHCP)
#error WIFI_ALLOW_GRACE has no effect if NO_DHCP is set
#endif

#define WIFI_DELAY_TRIGGER 500
#define WIFI_DELAY 100          //  100ms delay approved

extern _u32 wifi_delay;
_u32 wifi_delay_timer_start_reference;
_u32 wifi_delay_timer_stop_reference;

#endif  // WIFI_ALLOW_GRACE

static void
ur_handler_on_sta_got_ip(void *arg, esp_event_base_t event_base,
                      __i32 event_id, void *event_data)
{
TP05
#if defined(WIFI_ALLOW_GRACE)
    if (!wifi_delay) {
        wifi_delay_timer_stop_reference = tstamp();
        PR02("TP_gtip: %u\n", wifi_delay_timer_stop_reference);
        if (wifi_delay_timer_stop_reference - wifi_delay_timer_start_reference > WIFI_DELAY_TRIGGER) {
            wifi_delay = WIFI_DELAY;
            PR00("cfg wifi_delay to %d\n", wifi_delay);
        }
    }
#endif  // WIFI_ALLOW_GRACE
    wifi_conn_retry = 0;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
#if defined(DUMP_SOME)
    dump_ev(__func__, event);
    dump_some(WIFI_MODE_STA, __func__);
#endif
    if (!ur_is_our_netif(NETIF_DESC_STA, event->esp_netif)) {
        return;
    }
#if defined(DUMP_SOME)
    ur_print_all_netif_ips(NETIF_DESC_STA);
#endif
    sprintf(gw_ip, IPSTR, IP2STR(&event->ip_info.gw));     // signal gateway as initialized (no xSemaphore Given query needed)

#if defined(NO_DHCP)
    if (*cached_ip) {       
        PR02("received previously set static IP data in ur_handler_on_sta_got_ip() -> doing nothing\n");
    } else {
        // cache DHCP data if: not already done or cache marked invalid
        esp_netif_dns_info_t dns;
        ESP_ERROR_CHECK(esp_netif_get_dns_info(event->esp_netif, ESP_NETIF_DNS_MAIN, &dns));
        sprintf(cached_ip,      IPSTR, IP2STR(&event->ip_info.ip));
        sprintf(cached_gateway, IPSTR, IP2STR(&event->ip_info.gw));    
        sprintf(cached_subnet,  IPSTR, IP2STR(&event->ip_info.netmask));
        sprintf(cached_dns,     IPSTR, IP2STR(&dns.ip.u_addr.ip4));
        PR02("received DHCP data in ur_handler_on_sta_got_ip() -> caching\n");
    }
#if DEBUG > 1
    GS05(cached_ip);
    GS05(cached_gateway);
    GS05(cached_subnet);
    GS05(cached_dns);
#endif  
#endif  // NO_DHCP

    if (s_semph_get_ip_addrs) {
        xSemaphoreGive(s_semph_get_ip_addrs);
    } else {
        PR00("out of the blue IPv4 address: " IPSTR "\n", IP2STR(&event->ip_info.ip));
    }
}

#if defined(NO_DHCP)
static esp_err_t 
ur_set_dns_server(esp_netif_t *netif, __u32 addr, esp_netif_dns_type_t type)
{
TP05
    if (addr && (addr != IPADDR_NONE)) {
        esp_netif_dns_info_t dns;
        dns.ip.u_addr.ip4.addr = addr;
        dns.ip.type = IPADDR_TYPE_V4;
        ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, type, &dns));
    }
    return ESP_OK;
}

// stolen from [ esp-idf.master/examples/protocols/static_ip/main/static_ip_example_main.c ]
static void
ur_set_static_ip(esp_netif_t *netif)
{
TP05
    /*
     * ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED is the NORMAL case here: the netif was either created
     * without ESP_NETIF_DHCP_CLIENT, or we stopped the client on an earlier reconnect. Bailing out
     * on it meant the static IP and DNS below were NEVER applied, and since the default wifi
     * handlers clear the netif IP on every disconnect, the station ended up with no IP at all
     * from the second reconnect onwards (esp32-46 lockout, 2026_08_25)
     */
    esp_err_t dhcp_err = esp_netif_dhcpc_stop(netif);

    if (dhcp_err != ESP_OK && dhcp_err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
        PR00("Failed to stop dhcp client: 0x%x\n", dhcp_err);
        return;
    }
    esp_netif_ip_info_t ip;
    memset(&ip, 0 , _SZ(esp_netif_ip_info_t));
    ip.ip.addr = ipaddr_addr(cached_ip);
    ip.netmask.addr = ipaddr_addr(cached_subnet);
    ip.gw.addr = ipaddr_addr(cached_gateway);
    if (esp_netif_set_ip_info(netif, &ip) != ESP_OK) {
        /*
         * returning quietly here leaves the netif with NO usable address while *cached_ip still
         * claims one is valid - so every later association takes this same static branch and fails
         * again: precisely the lockout shape fixed on 2026_08_25. Invalidate, so the next
         * association falls into the DHCP branch instead. (hardening; never observed firing)
         */
        *cached_ip = 0;
        PR00("Failed to set ip info -> dropped cached DHCP data, will re-lease\n");
        return;
    }
    ESP_ERROR_CHECK(ur_set_dns_server(netif, ipaddr_addr(cached_dns), ESP_NETIF_DNS_MAIN));
}


/*
 * which AP instance the cached DHCP data AND the gateway MAC were learned from
 */
RTC_DATA_ATTR _u8 cached_ap_bssid[6];

#if defined(CACHE_GW_MAC)
RTC_DATA_ATTR _u8 cached_gw_mac[6];
RTC_DATA_ATTR _u8 cached_gw_mac_valid;

/*
 * both of these touch the raw lwIP ARP table, and CONFIG_LWIP_TCPIP_CORE_LOCKING is NOT set here,
 * so LOCK_TCPIP_CORE() would be a no-op -> they may only run on the tcpip thread itself, which is
 * what esp_netif_tcpip_exec() is for. calling them straight from the event or app task races the
 * stack
 */
static esp_err_t
ur_arp_prime(void *ctx)
{
    ip4_addr_t ip;
    struct eth_addr mac;

    ip.addr = ipaddr_addr(cached_gateway);
    memcpy(mac.addr, cached_gw_mac, _SZ(mac.addr));
    return etharp_add_static_entry(&ip, &mac) == ERR_OK ? ESP_OK : ESP_FAIL;
}

static esp_err_t
ur_arp_unprime(void *ctx)
{
    ip4_addr_t ip;

    ip.addr = ipaddr_addr(cached_gateway);
    etharp_remove_static_entry(&ip);    // harmless if there is none
    return ESP_OK;
}

static esp_err_t
ur_arp_learn(void *ctx)
{
    ip4_addr_t ip;
    struct eth_addr *mac = 0;
    const ip4_addr_t *ip_ret = 0;

    ip.addr = ipaddr_addr(cached_gateway);
    if (etharp_find_addr(0, &ip, &mac, &ip_ret) < 0 || !mac) {
        return ESP_FAIL;
    }
    memcpy(cached_gw_mac, mac->addr, _SZ(cached_gw_mac));
    cached_gw_mac_valid = 1;
    return ESP_OK;
}
#endif  // CACHE_GW_MAC
#endif  // NO_DHCP

static void
ur_handler_on_wifi_connect(void *esp_netif, esp_event_base_t event_base,
                            __i32 event_id, void *event_data)
{
TP05
#if defined(WIFI_ALLOW_GRACE)
    if (!wifi_delay) {
        wifi_delay_timer_start_reference = tstamp();
        PR02("TP_conn: %u\n", wifi_delay_timer_start_reference);
    }
#endif      // WIFI_ALLOW_GRACE

#if defined(CACHE_CHANNEL)
    /*
     * the fill USED TO sit in mysend() behind [ #if DEBUG > 1 ] and [ #if defined(DUMP_RSSI) ],
     * reading esp_wifi_sta_get_ap_info(). every entity builds with DEBUG 1, so cached_channel
     * stayed 0 forever and CACHE_CHANNEL was a silent no-op.
     *
     * WIFI_EVENT_STA_CONNECTED hands us bssid and channel directly, with no extra call, no DEBUG
     * level and no DUMP_RSSI coupling -> do it here
     */
    {
        wifi_event_sta_connected_t *ev = (wifi_event_sta_connected_t *)event_data;

        cached_channel = ev->channel;
        memcpy(cached_bssid, ev->bssid, _SZ(cached_bssid));
        PR02("caching BSSID " MACSTR " on channel %d\n", MAC2STR(cached_bssid), cached_channel);
    }
#endif

#if defined(NO_DHCP)

    //
    // if ESP_NETIF_DHCP_CLIENT was not set in ur_wifi_start()
    // we MUST ur_set_static_ip() here because otherwise the system
    // hangs forever. Since ur_handler_on_sta_got_ip() will never be called
    //
    /*
     * a changed BSSID means a DIFFERENT AP INSTANCE. An Android hotspot picks a new RANDOM SUBNET
     * every time it restarts - two entirely unrelated private subnets across one reboot - so
     * the cached lease is not merely stale, it belongs to a network that no longer exists.
     * Asserting it as a static IP leaves our ARP unanswered and every connect() times out.
     * Drop the lot here and let DHCP run (esp32-46, 2026_08_25)
     */
    {
        wifi_event_sta_connected_t *ev = (wifi_event_sta_connected_t *)event_data;

        if (memcmp(cached_ap_bssid, ev->bssid, _SZ(cached_ap_bssid))) {
            if (*cached_ip) {
                *cached_ip = 0;
                PR02("BSSID changed -> dropping cached DHCP data, forcing a fresh lease\n");
            }
#if defined(CACHE_GW_MAC)
            if (cached_gw_mac_valid) {
                cached_gw_mac_valid = 0;
                esp_netif_tcpip_exec(ur_arp_unprime, 0);
                PR02("BSSID changed -> dropping cached gateway MAC + static ARP entry\n");
            }
#endif
            memcpy(cached_ap_bssid, ev->bssid, _SZ(cached_ap_bssid));
        }
    }
    if (*cached_ip) {
        PR02("applying cached DHCP data in ur_handler_on_wifi_connect() -> setting static IP\n");
        ur_set_static_ip(esp_netif);
#if defined(CACHE_GW_MAC)
        /*
         * earliest point at which the gateway IP is configured, so prime its ARP entry here and
         * the first packet out (TCP SYN, or the DNS query without CACHE_TARGET_IP) goes straight
         * on the wire instead of waiting a round trip for the reply
         */
        if (cached_gw_mac_valid) {
            if (esp_netif_tcpip_exec(ur_arp_prime, 0) == ESP_OK) {
                PR02("primed static ARP entry for gateway %s\n", cached_gateway);
            } else {
                cached_gw_mac_valid = 0;
                PR02("could not prime ARP entry -> dropping cached gateway MAC\n");
            }
        }
#endif
    } else {
        /*
         * the old comment here said "do nothing, ur_wifi_start()'s ESP_NETIF_DHCP_CLIENT flag
         * refreshes this automatically". That only holds on the DEEP sleep path, where the netif
         * is recreated on every wakeup. On LIGHT SLEEP the netif persists, ur_wifi_start() never
         * runs again, and once ur_set_static_ip() has stopped the client NOTHING ever starts it
         * again -> no lease, forever, until the netif is destroyed (which is why picking a menu
         * key for another accesspoint was the only cure). start it explicitly (esp32-46).
         */
        esp_err_t dhcp_err = esp_netif_dhcpc_start(esp_netif);

        if (dhcp_err != ESP_OK && dhcp_err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED) {
            PR00("could not start dhcp client: 0x%x\n", dhcp_err);
        }
        PR02("no cached DHCP data in ur_handler_on_wifi_connect() -> dhcp client started\n");
    }
#endif      // NO_DHCP

}

#if defined(WIFI_RESTORE)
RTC_DATA_ATTR _u32 wifi_restored = 0;
#endif

void
#if defined(EARLY_WIFI_PS)
ur_wifi_start(_u32 ps_type)
#else
ur_wifi_start(void)
#endif
{
TP05
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();        // initializer I
#if defined(DUMP_SOME)
    dump_cfg(__func__, &cfg);
#endif

    /*
    in AP mode country settings and others are greatly running like this
    initialize_AP() in ultra_ap.c

    test something similar same for STA MODE HERE!!!!!!!!!!!!!!!!!!!!!!
    */

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    WTPROF("w_wifi_init");
#if defined(WIFI_RESTORE)
    if (!wifi_restored) {
        ESP_ERROR_CHECK(esp_wifi_restore());
        PR00("esp_wifi_restore() called\n");
#if 0
        ESP_ERROR_CHECK(esp_wifi_set_country_code("US", 0));
#else
        wifi_country_t country = {
            .cc = "US",
            .schan = 0x1,
            .nchan = 0xb,
            .max_tx_power = 0x1e,   // ignored
            .policy = WIFI_COUNTRY_POLICY_MANUAL,
        };
        ESP_ERROR_CHECK(esp_wifi_set_country(&country));
#endif
        PR00("esp_wifi_set_country_code() called\n");
        ++wifi_restored;
    }
#endif

    /*
     * values default to
     *
     * wifi values dump:
     * get_ps: 1 country [01 �] bw: 2
     *
     * if no association to any AP yet did happen
     * 
     * per default takes over AP values (if applicable). i.e. if doing nothin special gets in mysend():
     *
     * mysend wifi values:
     * get power save: 0 country [US ] bw: 2        <- values as of why: iw wlan0 scan dump | less
     *
     */
    WIFI_FIXER_DEBUG("wifi values dump:\n", WIFI_MODE_STA);
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();   // initializer II

#if defined(NO_DHCP)
    // undo the damage of ESP_NETIF_INHERENT_DEFAULT_WIFI_STA() if cached DHCP data is already avail
    if (*cached_ip) {
        esp_netif_config.flags &= ~ESP_NETIF_IPV4_ONLY_FLAGS(ESP_NETIF_DHCP_CLIENT);
        PR02("cached DHCP data avail in ur_wifi_start() -> clearing ESP_NETIF_DHCP_CLIENT\n");
    } else {
        PR02("no cached DHCP data avail in ur_wifi_start() -> setting ESP_NETIF_DHCP_CLIENT\n");
    }
#endif  // NO_DHCP

    esp_netif_config.if_desc = NETIF_DESC_STA;
    esp_netif_config.route_prio = 128;
    s_ur_sta_netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
    esp_wifi_set_default_wifi_sta_handlers();
    WTPROF("w_netif_create");

// ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_FLASH) );   TESTEN
// ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    /*
     * do NOT "optimize" this to WIFI_STORAGE_RAM / cfg.nvs_enable = 0 the way the ESPNOW path
     * did: libnet80211.a keeps sta.pmk in NVS, i.e. the WPA2 PBKDF2-SHA1(4096) result. killing
     * the driver's NVS access means re-deriving that on every single wakeup
     */
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    WTPROF("w_set_mode");
    ESP_ERROR_CHECK(esp_wifi_start());
    WTPROF("w_wifi_start");

#if defined(FORCE_2G4_BAND) && defined(CONFIG_SOC_WIFI_SUPPORT_5G)
    /*
     * after esp_wifi_start(): the API returns ESP_ERR_WIFI_NOT_STARTED before that.
     * guarded on SOC_WIFI_SUPPORT_5G because the band mode only means anything on a dual band
     * part - see the define
     */
    ESP_ERROR_CHECK(esp_wifi_set_band_mode(WIFI_BAND_MODE_2G_ONLY));
    WTPROF("w_band_2g4");
#endif

#if defined(EARLY_WIFI_PS)
    /*
     * before esp_wifi_connect(), so the association is not negotiated under the driver default
     * WIFI_PS_MIN_MODEM - see the define
     */
    ESP_ERROR_CHECK(esp_wifi_set_ps(ps_type));
    WTPROF("w_set_ps_early");
#endif

    /*
     * see also [ VALID FOR ESP32_(37) ] comment in arduino section for this
     */
    REDUCE_WIFI_POWER_IF_REQUIRED();
    WTPROF("w_wifi_power");
}

esp_err_t
ur_wifi_sta_do_connect(wifi_config_t wifi_config, bool wait)
{
TP05
    s_semph_get_ip_addrs = xSemaphoreCreateBinary();
    if (!s_semph_get_ip_addrs) {
        return ESP_ERR_NO_MEM;
    }
    wifi_conn_retry = 0;
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &ur_handler_on_wifi_disconnect, 0));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ur_handler_on_sta_got_ip, 0));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &ur_handler_on_wifi_connect, s_ur_sta_netif));
    WTPROF("w_handlers");
    /*
     * watch this one: a sta.pmk cache MISS makes the driver run PBKDF2-SHA1(4096) right here
     */
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    WTPROF("w_set_config");
    esp_err_t ret = esp_wifi_connect();
    WTPROF("w_connect_issued");
    if (ret != ESP_OK) {
        PR00("WiFi connect failed! ret:%x\n", ret);
        return ret;
    }
    if (wait) {
        xSemaphoreTake(s_semph_get_ip_addrs, portMAX_DELAY); // effective no timeout
        WTPROF("w_got_ip");
        if (wifi_conn_retry > wifi_conn_retry_max) {
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}


esp_err_t
#if defined(EARLY_WIFI_PS)
ur_wifi_connect(_u8 ssid, bool wait, _u32 ps_type)
#else
ur_wifi_connect(_u8 ssid, bool wait)
#endif
{
TP05
#if defined(NO_DHCP)
    if (ssid_last != ssid) {
        // accesspoint changes imply DHCP cache invalidation
        *cached_ip = 0;
#if defined(CACHE_CHANNEL)
        cached_channel = 0;     // invalidate this too for your safety
#endif
        beep(BEEP_ERR, 1);      // signal cache invalidation 
        PR02("clearing cached DHCP data due to SSID change\n");
    }
#endif  // NO_DHCP
    ssid_last = ssid;
#if defined(EARLY_WIFI_PS)
    ur_wifi_start(ps_type);
#else
    ur_wifi_start();
#endif
    wifi_config_t wifi_config = {
        .sta = {
//          .ssid = *WIFI4_SSID,
//          .password = *WIFI4_PASSWORD,
            .scan_method = WIFI_SCAN_METHOD,
            .sort_method = WIFI_CONNECT_AP_SORT_METHOD,
            .threshold.rssi = WIFI_SCAN_RSSI_THRESHOLD,
            .threshold.authmode = WIFI_SCAN_AUTH_MODE_THRESHOLD,
#if defined(CACHE_CHANNEL)
            .bssid_set = !!cached_channel,
#endif
        },
    };
    strcpy((_i8p)wifi_config.sta.ssid, GET_SSID(ssid));
    strcpy((_i8p)wifi_config.sta.password, GET_PASS(ssid));
#if 0
// test w/o password
    *(_i8p)wifi_config.sta.password = 0;
    wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
#endif
#if defined(CACHE_CHANNEL)
    if (cached_channel) {
        PR02("cached BSSID data avail %02x:%02x:%02x:%02x:%02x:%02x on channel %d\n", 
            cached_bssid[0], cached_bssid[1], cached_bssid[2],
            cached_bssid[3], cached_bssid[4], cached_bssid[5],
            cached_channel
        );
        wifi_config.sta.channel = cached_channel;
        memcpy(wifi_config.sta.bssid, cached_bssid, 6);
    } else {
        PR02("no cached BSSID data available\n");
    }
#endif      // defined(CACHE_CHANNEL)
    PR06("SSID: %s\n", wifi_config.sta.ssid);
    return ur_wifi_sta_do_connect(wifi_config, wait);
}

esp_err_t
ur_wifi_sta_do_disconnect(void)
{
TP05
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &ur_handler_on_wifi_disconnect));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &ur_handler_on_sta_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &ur_handler_on_wifi_connect));
    if (s_semph_get_ip_addrs) {
        vSemaphoreDelete(s_semph_get_ip_addrs);
    }
    return esp_wifi_disconnect();
}

void
ur_wifi_stop(void)
{
TP05
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(s_ur_sta_netif));
    esp_netif_destroy(s_ur_sta_netif);
    s_ur_sta_netif = 0;
}

void
ur_wifi_shutdown(void)
{
TP05
    *gw_ip = 0;         // flag as uninitialized
    ur_wifi_sta_do_disconnect();
#if defined(WIFI_DEAUTH_SETTLE_US) && WIFI_DEAUTH_SETTLE_US > 0
    /*
     * esp_wifi_disconnect() only QUEUES the deauth; ur_wifi_stop() right behind it kills the
     * radio. if the frame never makes it out, the AP keeps a stale association for us and answers
     * our next wakeup with deauth reason 2 AUTH_EXPIRE - measured at 4..7% of cycles against the
     * BPI-R3, each costing a fixed ~2.8s of w_link_up. give the frame a moment to leave
     */
    esp_rom_delay_us(WIFI_DEAUTH_SETTLE_US);
#endif
    ur_wifi_stop();
}

esp_err_t
ur_connect(_u8 ssid, bool wait, _u32 max_retries, _u32 type)
{
TP05
    PR06("ssid: %d wait: %d max_retries: %d power save: %s\n", ssid, wait, max_retries, give_ps(type));
    wifi_conn_retry_max = max_retries;
#if defined(EARLY_WIFI_PS)
    if (ur_wifi_connect(ssid, wait, type) != ESP_OK) {
#else
    if (ur_wifi_connect(ssid, wait) != ESP_OK) {
#endif
        return ESP_FAIL;
    }
    ESP_ERROR_CHECK(esp_register_shutdown_handler(&ur_wifi_shutdown));
#if !defined(EARLY_WIFI_PS)
    ESP_ERROR_CHECK(esp_wifi_set_ps(type));
#endif
    WTPROF("w_set_ps");
    PR02("TP01: %lu WiFi %s\n", tstamp(), wait ? "connected" : "issued");
    PR02("[ %s on %s ] ready to operate, WiFi stat: %s\n", "prg", HOST, "give_wifi_status(WiFi.status())");
    return ESP_OK;
}

esp_err_t
ur_upgrade_max_retries(_u32 max_retries)
{
TP05
    PR06("to max_retries: %d\n", max_retries);
    wifi_conn_retry_max = max_retries; 
    return ESP_OK;
}

esp_err_t
ur_disconnect(void)
{
TP05
    ur_wifi_shutdown();
    ESP_ERROR_CHECK(esp_unregister_shutdown_handler(&ur_wifi_shutdown));
    return ESP_OK;
}
#endif  // defined(WIFI_INITIATOR) || defined(WIFI_TARGET)
/* ---^^^--- WiFi section (idf runtime) ---^^^----------------------------------------------------------------- */


/* ---vvv--- cmd to status (idf mysend) ---vvv----------------------------------------------------------------- */
#if defined(WIFI_INITIATOR) || defined(ESPNOW_INITIATOR) || defined(ETH_INITIATOR)

#include "lwip/netdb.h"         // required for addrinfo in mcom.h
#include <fcntl.h>              // required by the non-blocking connect below

#define WIFI_PQCMD_TIMEOUT 1000     // x 1ms == 1s (account for server busy cmd timeout)
/*
 * per-attempt SYN timeout for the fast connect retry - see the big comment at the connect().
 * must stay well under WIFI_PQCMD_TIMEOUT so several tries fit inside it. against the BPI-R3 the
 * AP needs ~200ms before it carries the first frame, so expect a handful of tries there and
 * exactly one against an accesspoint that does not drop it
 */
#define WIFI_CONNECT_TRY_MS 20
#define WIFI_STATUS_TIMEOUT 5       // x 1s == 5s (no status received from server)

RTC_DATA_ATTR _u8 multi_gw_serial_no;

#if defined(CACHE_TARGET_IP)
/*
 * the resolved target address, kept across deep sleep exactly like cached_ip
 *
 * keyed on the host string AND (where there is one) the accesspoint, so a target change or an
 * SSID change re-resolves. invalidated together with cached_ip when connect() fails, which is
 * the same safety net the static IP relies on
 */
RTC_DATA_ATTR _i8 cached_target_host[48];
RTC_DATA_ATTR _u32 cached_target_addr;      // A record, network byte order. 0 == invalid
#if defined(WIFI_INITIATOR) || defined(WIFI_TARGET)
RTC_DATA_ATTR _u8 cached_target_ssid;
#endif
#endif  // CACHE_TARGET_IP
/*
 * a host zero string denotes the cmd wants to be sent via ESPNOW (and WiFi otherwise)
 */
/*
 * mysend() keeps its whole working set in FILE SCOPE globals: espnow_sol_pkt (which is both the
 * tcp send AND the receive buffer, and is held across the entire transaction), _pmatch[]/_regex
 * for the status parse, plus the function level static stat_misc_buf[] below. that was safe for
 * as long as exactly one task ever called it.
 *
 * ultra_espnow_gw has TWO callers. espnow_unsol_task forwards a non-pre-acked command inline;
 * pq_task forwards a pre-acked one asynchronously. drive both at once and they interleave inside
 * this function.
 *
 * measured on esp32-44, 2026_08_30: with both paths live, the gateway put pq_task's raw buffer on
 * a socket espnow_unsol_task had opened, truncated mid-string - the target logged the command as
 * "e45.p80 ^host2", which no single path can construct (espnow_unsol_task strips the host out
 * during parsing, so ^host2 cannot appear in what it sends). twice in three minutes. the device
 * then panicked, having been up 13.3 hours under single path load, and the panic was NOT at any
 * instrumented ESP_ERROR_CHECK site - i.e. a memory fault, which is what a _pmatch offset pair
 * computed across two different strings does to the strncat() below.
 *
 * so serialise the function. both callers are I/O bound, and the pre-ack has already gone back to
 * the remote before pq_task ever runs, so the wait is not observable from outside.
 *
 * the handle is created in init_1st(). projects that never call init_1st() leave it null and the
 * macros below are then no-ops - behaviour identical to before this change, which keeps every
 * single caller project exactly as it was.
 */
#if defined(GW_TIMING_TRACE)
/*
 * phase split of the last mysend() TCP transaction, us. file scope is safe here for exactly the
 * reason the mutex below exists: MYSEND_LOCK serialises the whole function, so there is only ever
 * one transaction in flight. reset per call - several "goto out1" paths leave phases unreached,
 * and a stale value from the previous command would read as a plausible measurement
 */
_u32 gw_t_connect, gw_t_write, gw_t_read;
#endif

SemaphoreHandle_t mysend_lock = 0;

#define MYSEND_LOCK()   do { if (mysend_lock) xSemaphoreTake(mysend_lock, portMAX_DELAY); } while (0)
#define MYSEND_UNLOCK() do { if (mysend_lock) xSemaphoreGive(mysend_lock); } while (0)

_i32
mysend(_i8cp cmd, _i8cp host, _u16 port, _i8p *statmsg)
{
TP05
    _i32 stat, err;
    _i32 s = -1;    // socket fd needs no close per default

    /*
     * taken here and released at the single exit below. every "goto out1" in between passes
     * through that exit, so there is exactly one take and one give on every path
     */
    MYSEND_LOCK();

#if defined(ESPNOW_INITIATOR)

/*
 * must dynamically switch between ESPNOW and traditional WiFi
 */
if (isESPNOW(host)) {

    /*
     * add a serial-no to allow tossing duplicates on target side preserving a possible ^instant 
     *
     * in case of NON-pre-acked cmds the serial-no is filtered on the final target (simu host2 bell here). e.g.
     *      "no ^host2.example.com:8899"
     *   the target executes the first copy and drops the rest, answering a duplicate with the SAME
     *   benign status as the real execution (bell: accept_close(.., 0), "may not send this as first
     *   status") - so whichever of the three gateways' replies reaches the initiator first carries
     *   the same thing, and there is nothing to filter here. verified on bell 2026_09_15
     *
     * in case of pre-acked cmds the serial-no is filtered on esp32_decode/tcp_server
     *      "@beep= f:1000 c:1 t:.05 p:.25 g:-20 ^host2.example.com:8899 ^"
     *   duplicates are all exact the same STAT_OK and prevent to overwrite per if (!espnow_sol_pkt.len) { check
     */

    _i8 cmdSN[100];
    _u8 ptr;

    if (NUM_ESPNOW_GATEWAYS > 1) {
        PR05("multi antenna espnow cmd -> adding serial\n");
        strcpy(cmdSN, cmd);
        ptr = strlen(cmdSN) - 1;
        if (cmdSN[ptr] == '^') {
            snprintf(cmdSN + ptr, _SZ(cmdSN) - ptr, "^%dS ^", multi_gw_serial_no); // overwrites the last char '^' at ptr position
        } else {
            snprintf(cmdSN + ptr + 1, _SZ(cmdSN) - ptr - 1, " ^%dS", multi_gw_serial_no); // overwrites the limiting zero char
        }
        ++multi_gw_serial_no;
        cmd = cmdSN;
    }
    /*
     * code path for ESPNOW only
     *
     * e.g.
     * ESPNOWcmd: 145 [@beep= f:1000 c:1 t:.05 p:.25 g:-20 ^host2 ^] len 43 -> espnow:0
     * i.e. NO trailing '\n'
     */
    PR01("espnow cmd: %lu [%s] len %d -> gateway\n", tstamp(), cmd, strlen(cmd));
    espnow_sol_pkt.len = 0;     // indicate buffer free for cb routines
    xEventGroupClearBits(espnow_sol_events, ESPNOW_EVENT_RESPONSE_MASK);

    /*
     * cmd has no trailing '\n" but is sent inclusive the terminating null / also reflected in pkt.len
     * all gateway_macs are addressed in a loop -> this will produce duplicates on the receiving side:
     *  these will be filtered in esp32_decode/tcp_server on host1 (if cmd is pre-acked)
     * or
     *  by target application like bell (if cmd is sent directly to a target, to gain real (AKA non-pre-acked status))
     * 
     * CAVEAT:
     *  pre-acked commands receive all a uniform status -> any of these statuses is good to be sent back
     * BUT
     *  non-pre-acked commands must be tagged directly on their target application (e.g. bell.c) to allow identifying the REAL return status
     *  filtering duplicates takes place in ultra_espnow_gw.c/espnow_unsol_task()/forward_cmd()/statmsg
     * 
     * this ensures only valid status data is queued if xEventGroupWaitBits() triggers
     */
    EventBits_t bits;
    for (_i32 tries = 0; ; ++tries) {           // signed: with the default 0, >= would be always true
        for (_u32 i = 0; i < NUM_ESPNOW_GATEWAYS; ++i) {
            ESP_ERROR_CHECK(esp_now_send(espnow_gateway_mac[i], (_u8p)cmd, strlen(cmd) + 1)); // copy terminating 0
            PR02("esp_now_send %d " MACSTR "\n", i, MAC2STR(espnow_gateway_mac[i]));
        }
        bits = xEventGroupWaitBits(
            espnow_sol_events,
            ESPNOW_EVENT_RESPONSE_MASK,
            pdTRUE,
            pdFALSE,
            pdMS_TO_TICKS(ESPNOW_STATUS_TIMEOUT)
        );
        if ((bits & ESPNOW_EVENT_RESPONSE_MASK) || tries >= ESPNOW_RESEND_UNANSWERED)
            break;
        /*
         * not one gateway answered: send it again, unchanged. a copy that did get through the
         * first time is answered as a duplicate with the same status, so nothing runs twice
         */
        PR01("espnow stat: no answer, sending [%s] again\n", cmd);
        espnow_sol_pkt.len = 0;                 // indicate buffer free for cb routines
        xEventGroupClearBits(espnow_sol_events, ESPNOW_EVENT_RESPONSE_MASK);
    }

    /*
     * the status received contains a trailing '\n' and is sent without terminating null
     * to match historic mysend behavior
     */
    if (bits & ESPNOW_EVENT_RESPONSE_MASK) {
        // any of the possible RCV packets alone is sufficient to cancel the timeout of all
        // care must be taken to toss duplicates right before coming here:
        //  for pre-acked cmds tcp_server does the hard work
        //  for non-pre-acked cmds the target application must handle this
    } else {
        PR01("espnow stat: timeout\n");
        beep(BEEP_INFO, 1);     // provide some accoustic feedback
    }
} else {

#endif // if defined(ESPNOW_INITIATOR)

//---vvv-------------- WiFi / no ETH --------------------
#if !defined(ETH_OPMODE)

    WTPROF("w_send_entry");
    PR01("wifi cmd: %lu %s -> [%s:%u]\n", tstamp(), cmd, host, port);
    /*
     * in case of KARR_TARGET_HOST (and other smartphones) there is no fixed host
     * as it changes with currently selected gateway IP
     */
    if (!strcmp(host, KARR_TARGET_HOST)) host = gw_ip;

#else
//---^^^-------------- WiFi / no ETH --------------------

    PR01("eth cmd: %lu %s -> [%s:%u]\n", tstamp(), cmd, host, port);
#endif

    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    _u32 last;

//---vvv-------------- WiFi / no ETH --------------------
#if !defined(ETH_OPMODE) 
    /*
     * code path for WiFi / no ETH
     */
    if (!*gw_ip) {  // flagged still as uninitialized, so wait
        xSemaphoreTake(s_semph_get_ip_addrs, portMAX_DELAY); // effective no timeout
        /*
         * everything between w_connect_issued and here is scan + auth + assoc + 4-way handshake
         * + (NO_DHCP) the static IP set in ur_handler_on_wifi_connect() -> the prime suspect
         */
        WTPROF("w_link_up");
#if defined(LINK_SETTLE_US) && LINK_SETTLE_US > 0
        /*
         * ACCESSPOINT_CONNECT_BOOSTER, idf flavour - see the define. same spot arduino used it:
         * link reports up, we hold off a moment, THEN put the first frame on the air
         */
        /*
         * ALWAYS the busy wait, never vTaskDelay: at FREERTOS_HZ 100 a one tick delay only runs
         * to the NEXT TICK BOUNDARY, so a nominal 10ms arm actually measured 165us..9.6ms across
         * cycles and told us nothing. wifi (prio 23) and lwip (18) preempt app_main (1) anyway
         */
        esp_rom_delay_us(LINK_SETTLE_US);
        WTPROF("w_link_settle");
#endif
        if (wifi_conn_retry > wifi_conn_retry_max) {
            stat = 1;
#if defined(CACHE_CHANNEL)
            /*
             * MANDATORY, not cosmetic: bssid_set pins us to exactly one AP, so an AP that moved
             * channel, changed MAC or went away would fail here on every future wakeup as well.
             * this is the only place that unpins it
             */
            cached_channel = 0;
            PR02("clearing cached BSSID/channel due to association failure\n");
#endif
            goto out1;
        }
#if DEBUG > 1
        PR00("TP02: %lu WiFi %s\n", tstamp(), "connected");
#if defined(DUMP_RSSI)
        wifi_ap_record_t ap_info;
        ESP_ERROR_CHECK(esp_wifi_sta_get_ap_info(&ap_info));
        PR00("RSSI: %d ", ap_info.rssi);
        wifi_country_t w_country;
        ESP_ERROR_CHECK(esp_wifi_get_country(&w_country));
        PR00(CCSTR "0x%02x 0x%02x\n",
            CC2STR(w_country.cc),
            w_country.max_tx_power,
            w_country.policy
        );

        /*
         * values default to
         *
         * wifi values dump:
         * get_ps: 1 country [01 �] bw: 2
         *
         * if no association to any AP yet did happen
         */
        WIFI_FIXER_DEBUG("mysend wifi values:\n", WIFI_MODE_STA);
#endif
#endif
    } else if (!strcmp(gw_ip, "0")) {   // "0" is special 'IP' indicating error for: permanently unavail
        stat = 1;
        goto out1;
    }
#endif // if !defined(ETH_OPMODE)
//---^^^-------------- WiFi / no ETH --------------------

    /*
     * common code path for INITIATORS ETH, WiFi, ESPNOW
     */
    _i8 _port[8]; sprintf(_port, "%d", port);
    /*
     * on a cache hit res stays 0 and target_sa carries the address instead
     */
    struct sockaddr_in target_sa;
    memset(&target_sa, 0, _SZ(target_sa));
    res = 0;

#if defined(CACHE_TARGET_IP)
    if (cached_target_addr
     && !strcmp(cached_target_host, host)
#if defined(WIFI_INITIATOR) || defined(WIFI_TARGET)
     && cached_target_ssid == ssid_last
#endif
    ) {
        target_sa.sin_family = AF_INET;
        target_sa.sin_port = htons(port);
        target_sa.sin_addr.s_addr = cached_target_addr;
        PR02("cached target address avail for [%s] -> skipping getaddrinfo()\n", host);
    } else {
#endif  // CACHE_TARGET_IP
        err = getaddrinfo(host, _port, &hints, &res);
        if (err || !res) {
            PR00("DNS lookup failed err=%d res=%p\n", err, res);
            stat = 2;
            goto out1;
        }
#if defined(CACHE_TARGET_IP)
        if (strlen(host) < _SZ(cached_target_host)) {
            strcpy(cached_target_host, host);
            cached_target_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
#if defined(WIFI_INITIATOR) || defined(WIFI_TARGET)
            cached_target_ssid = ssid_last;
#endif
            PR02("caching target address for [%s]\n", host);
        }
    }
#endif  // CACHE_TARGET_IP
    WTPROF("w_dns");
#if defined(GW_TIMING_TRACE)
    gw_t_connect = gw_t_write = gw_t_read = 0;
    _u64 gw_phase = esp_timer_get_time();
#endif
    last = tstamp();
    _u32 conn_try = 0;
    while (1) {
        /*
         * hints pin AF_INET/SOCK_STREAM, so this is what getaddrinfo() hands back anyway
         */
        s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0) {
            PR00("... Failed to allocate socket.\n");
            if (res) freeaddrinfo(res);
            stat = 2;
            goto out1;
        }

        /*
         * FAST CONNECT RETRY (2026_08_15) - do NOT go back to a plain blocking connect().
         *
         * measured against a BPI-R3/hostapd AP: the FIRST data frame after association is dropped
         * (the AP needs ~200ms before it carries it, see LINK_SETTLE_US). a blocking connect()
         * then sits out lwIPs own SYN retransmit, whose initial RTO is 3 tcp_slowtmr ticks of
         * 500ms - measured as w_tcp_connect 1,246,013..1,389,561us on EVERY cycle.
         *
         * the retry loop below could never help with that: blocking connect() does not RETURN
         * until that RTO has expired, so it was already too late. so run the connect
         * non-blocking and apply our own short timeout instead - a dropped SYN now costs
         * WIFI_CONNECT_TRY_MS, not 1.35s, and a healthy AP pays nothing at all
         */
        _i32 sock_fl = fcntl(s, F_GETFL, 0);
        fcntl(s, F_SETFL, sock_fl | O_NONBLOCK);

        ++conn_try;
        _u8 timed_out = 0;
        _i32 conn_ret = connect(s, res ? res->ai_addr    : (struct sockaddr *)&target_sa,
                                   res ? res->ai_addrlen : (socklen_t)_SZ(target_sa));
        if (conn_ret && errno == EINPROGRESS) {         // the normal case for a non-blocking connect
            fd_set wfds;
            struct timeval tv = { .tv_sec = 0, .tv_usec = WIFI_CONNECT_TRY_MS * 1000 };

            FD_ZERO(&wfds);
            FD_SET(s, &wfds);
            if (select(s + 1, 0, &wfds, 0, &tv) > 0) {
                _i32 so_err = 0;
                socklen_t so_len = _SZ(so_err);

                if (!getsockopt(s, SOL_SOCKET, SO_ERROR, &so_err, &so_len) && !so_err) {
                    conn_ret = 0;                       // up
                } else {
                    errno = so_err;
                }
            } else {
                errno = ETIMEDOUT;                      // our timeout, NOT lwIPs - retry at once
                timed_out = 1;
            }
        }
        if (conn_ret) {
            PR02("... connect try %lu failed errno=%d at %lu\n", (unsigned long)conn_try, errno, tstamp());
            if (WIFI_PQCMD_TIMEOUT > tstamp() - last) {
                close(s);
                if (!timed_out) {
                    /*
                     * only needed when the peer refused INSTANTLY (hot spin guard). after our own
                     * select() timeout we have already waited WIFI_CONNECT_TRY_MS, and sleeping
                     * more just coarsens how closely we can land on the moment the AP is ready
                     */
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                continue;
            }
            PR00("... socket connect failed errno=%d after %lu tries at %lu\n",
                errno, (unsigned long)conn_try, tstamp());
            if (res) freeaddrinfo(res);
            stat = 2;

#if defined(CACHE_TARGET_IP)
            /*
             * same reasoning as the cached DHCP data right below: a wrong address here is
             * exactly what a stale cache looks like, so drop it and re-resolve next time
             */
            cached_target_addr = 0;
            PR02("clearing cached target address due to socket connect() fail\n");
#endif
#if defined(CACHE_GW_MAC_ACTIVE)
            /*
             * a primed ARP entry is STATIC: lwIP never ages it out and never re-resolves it, so
             * clearing our own flag is NOT enough - the entry itself has to go, or every packet
             * to the gateway is black holed until the netif is destroyed. on the DEEP sleep path
             * that happens every cycle and hid the bug; on the LIGHT sleep path the netif lives
             * on and the remote is locked out forever (found on esp32-46, 2026_08_25)
             */
            cached_gw_mac_valid = 0;
            esp_netif_tcpip_exec(ur_arp_unprime, 0);
            PR02("clearing cached gateway MAC + static ARP entry due to socket connect() fail\n");
#endif

#if defined(NO_DHCP)
            /*
             * FIXME ALERT:
             *
             * problem: wrong DHCP IP in cache results in "no route to host":
             *
             * in the event of "no route to host" (e.g after smartphone reboot)
             * a connect() takes up to 31s to timeout?! -> how to shorten this??
             * SOLVED 2026_08_15 as a side effect of the fast connect retry above: the connect is
             * non-blocking now, so WIFI_PQCMD_TIMEOUT really does bound this at 1s
             *
             * typically we reach this code path if static IP stuff
             * makes wrong assumptions. so as a first resort we try to 
             * temporarily reanimate dynamic IP code (aka DHCP)
             */
            *cached_ip = 0;
#if defined(CACHE_CHANNEL)
            cached_channel = 0;     // invalidate this too for your safety
#endif
            beep(BEEP_ERR, 1);      // signal cache invalidation 
            PR02("clearing cached DHCP data due to socket connect() fail\n");
#endif  // NO_DHCP

            goto out1;
        }
        fcntl(s, F_SETFL, sock_fl);     // back to blocking: write()/read() below rely on it,
                                        // and read() gets its own SO_RCVTIMEO
        if (conn_try > 1) {
            PR01("tcp connect took %lu tries\n", (unsigned long)conn_try);
        }
//int flag = 1;
//setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, _SZ(int));
        break;
    }
    if (res) freeaddrinfo(res);
    WTPROF("w_tcp_connect");
#if defined(GW_TIMING_TRACE)
    { _u64 now = esp_timer_get_time(); gw_t_connect = (_u32)(now - gw_phase); gw_phase = now; }
#endif

    /*
     * IMPORTANT:
     * even though data is not necessarily coming from
     * espnow circuitry at this point we use data buffers with ESPNOW naming anyway
     * because we also must handle real ESPNOW data from above (hybrid model)
     */

    /*
     * trailing "\n" required, gets err: "invalid read count and/or unterminated data" otherwise
     */
    snprintf((_i8p)espnow_sol_pkt.data, _SZ(espnow_sol_pkt.data), "%s\n", cmd);
    if (write(s, espnow_sol_pkt.data, strlen((_i8p)espnow_sol_pkt.data)) < 0) {
        PR00("... socket send failed\n");
        stat = 3;
        goto out1;
    }
    WTPROF("w_tcp_write");
#if defined(GW_TIMING_TRACE)
    { _u64 now = esp_timer_get_time(); gw_t_write = (_u32)(now - gw_phase); gw_phase = now; }
#endif
    // bail if no status received
    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = WIFI_STATUS_TIMEOUT;
    receiving_timeout.tv_usec = 0;
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout, _SZ(receiving_timeout)) < 0) {
        PR00("... failed to set socket receiving timeout\n");
        stat = 3;
        goto out1;
    }
    err = read(s, espnow_sol_pkt.data, _SZ(espnow_sol_pkt.data) - 1);      // allow for appending a zero char
    if (err < 0) {
        PR00("... read failed\n");
        stat = 3;
        goto out1;
    } else {
        espnow_sol_pkt.len = err;
    }
    WTPROF("w_tcp_read");
#if defined(GW_TIMING_TRACE)
    { _u64 now = esp_timer_get_time(); gw_t_read = (_u32)(now - gw_phase); gw_phase = now; }
#endif

#if defined(CACHE_GW_MAC_ACTIVE)
    /*
     * deliberately AFTER the status is in, so learning costs nothing before stat. the exchange
     * just succeeded, so the gateway is certain to be in the ARP table right now
     */
    if (!cached_gw_mac_valid && esp_netif_tcpip_exec(ur_arp_learn, 0) == ESP_OK) {
        PR02("learned gateway %s MAC " MACSTR "\n", cached_gateway, MAC2STR(cached_gw_mac));
    }
    WTPROF("w_arp_learn");
#endif

#if defined(ESPNOW_INITIATOR)
}
#endif

    /*
     * IMPORTANT:
     * even though data is not necessarily coming from 
     * espnow circuitry at this point we use data buffers with ESPNOW naming anyway
     * because we also must handle real ESPNOW data from above (hybrid model)
     */

    /*
     * common code path for INITIATORS ETH, WiFi, ESPNOW
     *
     * the status received contains a trailing '\n' and is NOT null terminated
     * to match historic mysend behavior -> patch position [espnow_sol_pkt.len - 1]
     *
     * UPDATE as of 2026_07_09:
     * the status returned by 'p-process' fed through tcp_server
     * ALTERNATIVELY?!?! ends with ']' or with '\n' and is NOT null terminated -> patch/append as appropriate
     *
     */
    static _i8 stat_misc_buf[128];

    *stat_misc_buf = 0;
    if (espnow_sol_pkt.len > 0 
     && (espnow_sol_pkt.data[espnow_sol_pkt.len - 1] == '\n' || espnow_sol_pkt.data[espnow_sol_pkt.len - 1] == ']')) {
        if (espnow_sol_pkt.data[espnow_sol_pkt.len - 1] == '\n') {
            espnow_sol_pkt.data[espnow_sol_pkt.len - 1] = 0;    // overwrite

PR05("RECEIVED TRAILING '\\n'\n");
        } else {
            espnow_sol_pkt.data[espnow_sol_pkt.len] = 0;        // append (so read buflen - 1 max, see above)

PR05("RECEIVED TRAILING ']'\n");
        }
#if DEBUG 
#if defined(ESPNOW_INITIATOR)
if (isESPNOW(host)) {
        // f...... stuff must be casted explicitly since [ _i8 != (signed char) ] for this i....
        PR00("stat: %lu %s, rssi %d\n", tstamp(), espnow_sol_pkt.data, (signed char)espnow_sol_pkt.rssi); 
} else {
#endif  // if defined(ESPNOW_INITIATOR)
        PR00("stat: %lu %s\n", tstamp(), espnow_sol_pkt.data);
#if defined(ESPNOW_INITIATOR)
}
#endif  // if defined(ESPNOW_INITIATOR)
#endif  // if DEBUG
        if (err = regexec(&_regex, (_i8p)espnow_sol_pkt.data, _NE(_pmatch), _pmatch, 0)) {
            _i8 _buf[128];

            // no match
            regerror(err, &_regex, _buf, _SZ(_buf));
            PR02("%s\n", _buf);
            stat = 4;
            /*
             * this version:
             *  - leaves good status (==0) + STAT_MISC field as is
             *  - overwrites bad status (==1) with 5 and leaves STAT_MISC as is
             * to keep fail information in STAT_MISC available for further evaluation
             */
        } else if (strncmp("0", (_i8p)espnow_sol_pkt.data + _pmatch[STAT_STAT].rm_so, 1)) {
            PR02("status failed\n");
            stat = 5;
        } else {
            stat = 0;
        }
        strncat(stat_misc_buf, (_i8p)espnow_sol_pkt.data + _pmatch[STAT_MISC].rm_so, _pmatch[STAT_MISC].rm_eo - _pmatch[STAT_MISC].rm_so);
    } else {
        PR05("espnow_sol_pkt.len: %d\n", espnow_sol_pkt.len);
#if DEBUG > 4
        if (espnow_sol_pkt.len > 0) {
            PR00("espnow_sol_pkt.data[espnow_sol_pkt.len - 1]: '%c'\n", espnow_sol_pkt.data[espnow_sol_pkt.len - 1]);
        }
#endif
        PR00("invalid read count and/or unterminated data\n");
        stat = 3;
    }
out1:
    if (s >= 0) close(s);
    if (!stat) {
        if (!*stat_misc_buf) strcpy(stat_misc_buf, _err[stat]); // OK status
        beep(BEEP_OK, 1);
    } else if (stat == 5) {
        if (!*stat_misc_buf) strcpy(stat_misc_buf, _err[stat]);
        beep(BEEP_ERR, stat);
    } else {
        strcpy(stat_misc_buf, _err[stat]);
        beep(BEEP_ERR, stat);
    }
    PR02("statmsg: %s\n", stat_misc_buf);
    if (statmsg) *statmsg = stat_misc_buf;
    MYSEND_UNLOCK();
    return stat;
}
#endif  // if defined(WIFI_INITIATOR) || defined(ESPNOW_INITIATOR) || defined(ETH_INITIATOR)
/* ---^^^--- cmd to status (idf mysend) ---^^^----------------------------------------------------------------- */


/* ---vvv--- service control port (idf myserv) ---vvv---------------------------------------------------------- */
#if defined(MYSERVICE_PORT)

/*
 * as of bin/p 2022_10_24
 *
 * func server_status(cmd, stat, misc \
 *    )
 * {
 *    misc = misc ? misc : 0
 *    stat = stat ? stat : 0
 *    if (stat) beep(BEEP_ERR, 3)
 *    if (cmd != M["player"] A["init"]) {
 *        PR(strftime("%T: ") "-----------[ " cmd " - " (stat ? "NOK" : "OK ") "]------------")
 *    }
 *    return "#["     cmd      "]" \
 *           "#[" !!IS_REPEAT  "]" \
 *           "#[" !!IS_STEALTH "]" \
 *           "#["   PLAY_MODE  "]" \
 *           "#["     misc     "]" \         # <- retval for variables
 *           "#["     stat     "]"
 * }
 *
 *
 * 17570 toh@host2[/home/toh] > echo @OTA | nc esp32-2 8888
 * #[ar]#[0]#[0]#[0]#[-78]#[0]
 *
 */

/*
 * update as of 2026_03_06: now appends \n directly
 */
_i8p
statusStr(_i8p cmd, _i8 stat, _i8p misc, _i8p str, _u32 siz)
{
    snprintf(str,
             siz,
             "#[" "%s" "]#[" "0" "]#[" "0" "]#[" "0" "]#[" "%s" "]#[" "%s" "]\n",
             cmd, misc, stat ? "1" : "0"
    );
    return str;
}
#endif
/* ---^^^--- service control port (idf myserv) ---^^^---------------------------------------------------------- */


/* ---vvv--- initialization (idf variant) ---vvv--------------------------------------------------------------- */
RTC_DATA_ATTR _u32 bootCount = 0;

_i32
init_1st()
{
TP05
    /*
     * the recovery initArduino() ran ahead of every ard sketch: a partition without a free page, or one
     * written by a newer nvs format, is erased and initialized once more. any other error, and a second
     * failure, still aborts. without it an ard device updated OTA onto an idf image, or one whose nvs
     * filled up, aborts here and reboots into the same abort. the erase costs what nvs held: the wifi
     * cfg and pmk cache, skip_fw_update
     */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        PR00("nvs_flash_init() failed: 0x%x -> erasing nvs\n", err);
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ++bootCount;
#if defined(WIFI_INITIATOR) || defined(ESPNOW_INITIATOR) || defined(ETH_INITIATOR)
    /*
     * before any task exists, so no race to create it. see the comment at mysend()
     */
    if (!mysend_lock) mysend_lock = xSemaphoreCreateMutex();
#endif
    return 0;
}

_i32
init_2nd()
{
TP05
    PR02("bootCount: %d\n", bootCount);
#if DEBUG > 5
    PR00("esp-idf: %d.%d.%d\n", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
#if defined(ESP_ARDUINO_VERSION_MAJOR)
    PR00("ard-esp32: %d.%d.%d\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
#endif
    PR00("DEBUG: %d\n", DEBUG);
    PR00("HOST: %s\n", HOST);
    PR00("FW: %s\n", DEVICE_FW);

    const esp_app_desc_t *app_desc_ptr;
    app_desc_ptr = esp_app_get_description();
    GS05(app_desc_ptr->project_name);
    GS05(app_desc_ptr->time);
    GS05(app_desc_ptr->date);
    GS05(app_desc_ptr->idf_ver);

    print_reset_reason(0);
    print_reset_reason(1);
    print_wakeup_reason();
#endif
#if DEBUG > 1

    /*
     * // type
     * #define PART_TYPE_APP 0x00
     * 
     * // sub type
     * #define PART_SUBTYPE_FACTORY  0x00
     * #define PART_SUBTYPE_OTA_FLAG 0x10
     * #define PART_SUBTYPE_OTA_MASK 0x0f
     * #define PART_SUBTYPE_TEST     0x20
     * 
     * // ota state
     * ESP_OTA_IMG_NEW             = 0x0U,          Monitor the first boot. In bootloader this state is changed to ESP_OTA_IMG_PENDING_VERIFY. 
     * ESP_OTA_IMG_PENDING_VERIFY  = 0x1U,          First boot for this app was. If while the second boot this state is then it will be changed to ABORTED. 
     * ESP_OTA_IMG_VALID           = 0x2U,          App was confirmed as workable. App can boot and work without limits. 
     * ESP_OTA_IMG_INVALID         = 0x3U,          App was confirmed as non-workable. This app will not selected to boot at all. 
     * ESP_OTA_IMG_ABORTED         = 0x4U,          App could not confirm the workable or non-workable. In bootloader IMG_PENDING_VERIFY state will be changed to IMG_ABORTED. This app will not selected to boot at all. 
     * ESP_OTA_IMG_UNDEFINED       = 0xFFFFFFFFU,   Undefined. App can boot and work without limits. 
     */

    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    esp_ota_img_states_t ota_state;

    if (running != configured) {
        PR00("==================== running and configured different ====================\n");
    }
    PR06("actual OTA partition type %d subtype %d offset 0x%08lx ", running->type, running->subtype, running->address);
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        PR00("ota_state: 0x%x\n", ota_state);
    } else {
        PR00("ota_state: NOT AVAIL\n");
    }
#if 0
    if (running->address == 0x00210000) {
        esp_ota_mark_app_invalid_rollback_and_reboot();
        // esp_ota_mark_app_valid_cancel_rollback();
    }
#endif
#endif  // if DEBUG > 1
#if defined(WIFI_INITIATOR) || defined(ESPNOW_INITIATOR) || defined(ETH_INITIATOR)
    _i32 err;
    if (err = regcomp(&_regex, STATUS_MATCH, REG_EXTENDED)) {
        _i8 _buf[128];

        regerror(err, &_regex, _buf, _SZ(_buf));
        PR00("%s\n", _buf);
    }
#endif  // if defined(WIFI_INITIATOR) || defined(ESPNOW_INITIATOR) || defined(ETH_INITIATOR)
#if defined(BUZZER)
    beep_init();
    if (bootCount == 1) {
        beep(BEEP_INFO, 2);
    }
#endif
/*
 * VBAT_DEFER_INIT: adc_oneshot_new_unit()/adc_cali_create_scheme_curve_fitting() measure 40ms on
 * C5 against 1ms on ESP32 -> callers that care about wakeup latency init the ADC themselves,
 * right before they sample, instead of paying for it ahead of the cmd
 */
#if defined(VBAT_ADC1_SENSE_PIN) && !defined(VBAT_DEFER_INIT)
    vbat_monitor_init();
#endif
#if defined(RGB_GPIO_NUM)
    ws2812_init();
#endif
#if defined(LED_GPIO_NUM)
    led_init();        
#endif

/*
 * last chance to set the BASE MAC if needed
 *
 *  esp32-61 fakes esp32-44
 *
 */
#if ESP32_(61111)
_u8 espnow_base_mac_addr[] = ESPNOW_TOH_GW_MAC;
    ESP_ERROR_CHECK(esp_base_mac_addr_set(espnow_base_mac_addr));
#endif
    return 0;
}

_i32
deinit()
{
TP05
#if defined(VBAT_ADC1_SENSE_PIN)
    vbat_monitor_deinit();
#endif
#if defined(BUZZER)
    beep_deinit();
#endif
    return 0;
}
/* ---^^^--- initialization (idf variant) ---^^^--------------------------------------------------------------- */


/* ---vvv--- ESPNOW ur_connect() replacement initialization (idf variant) ---vvv------------------------------- */
/*
 * replacement for ur_connect() if full blown WiFi is not in use
 */
#define ESPNOW_WIFI_SETUP() \
do { \
/*  ESPNOW needs no netif: esp_wifi_init() + esp_wifi_start() alone carry esp_now_send() */ \
/*  creating one costs a DHCP client and the default STA handlers on every single wakeup */ \
/*    s_espnow_sta_netif = esp_netif_create_default_wifi_sta(); */ \
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); \
/*  WIFI_INIT_CONFIG_DEFAULT() sets nvs_enable=1 -> the driver is a second NVS consumer beside */ \
/*  the PHY cal cache. ESPNOW stores no config at all, so cut it here as well as via set_storage */ \
    cfg.nvs_enable = 0; \
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); \
/*  FLASH storage sends esp_wifi_init()/esp_wifi_start() to NVS and can auto-connect a stored STA cfg */ \
/*  ESPNOW never needs persisted credentials -> RAM (ur_wifi_start() keeps FLASH for the OTA path)    */ \
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM)); \
/*    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH)); */ \
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); \
    ESP_ERROR_CHECK(esp_wifi_start()); \
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE)); \
PR02("ESPNOW_CHANNEL: %d\n", ESPNOW_CHANNEL); \
    REDUCE_WIFI_POWER_IF_REQUIRED(); \
} while (0)

/*
 * replacement for ur_disconnect() if full blown WiFi is not in use
 */
#define ESPNOW_WIFI_UNSETUP() \
do { \
    ESP_ERROR_CHECK(esp_wifi_stop()); \
    ESP_ERROR_CHECK(esp_wifi_deinit()); \
/*  nothing to tear down while ESPNOW_WIFI_SETUP() creates no netif */ \
/*  if (s_espnow_sta_netif) { */ \
/*      esp_netif_destroy_default_wifi(s_espnow_sta_netif); */ \
/*      s_espnow_sta_netif = 0; */ \
/*  } */ \
} while (0)
/* ---^^^--- ESPNOW ur_connect() replacement initialization (idf variant) ---^^^------------------------------- */
/* ---^^^--- idf specific ---^^^------------------------------------------------------------------------------- */

#endif    // defined(MCOM_ARD)
#endif    // !defined(_MCOM_MINIMAL_)

