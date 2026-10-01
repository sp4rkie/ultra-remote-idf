// ---vvv--- standard includes ---vvv--- 
#include "sdkconfig.h"
#include "mnta.h"

// as of 2026_03_03 is standard for all
#define ESPNOW_INITIATOR
#define WIFI_INITIATOR
// as of 2026_07_02 is standard for all WOW!
#define MULTI_ANTENNA_GATEWAY 

/*
 * define the most common uremote class 
 * the first hardware built for this was ESP32_(15) 
 * hence the name
 * ESP32_(15) has been LOST!!! on channel track - so you won't find it any longer here
 */
// --- 14c/d/e/f/g/h button remote control [ No. 5/6/7/9/10/11 ] ---
// --- 16c button remote control [ No. 8 ] ---
#define ENTITY_15_CLASS (ESP32_(13) || ESP32_(14) || ESP32_(21) || ESP32_(47) || ESP32_(48) || ESP32_(49))

// S3 super mini accu versions
#define ENTITY_45_CLASS (ESP32_(45) || ESP32_(46) || ESP32_(82) || ESP32_(83) || ESP32_(84))

// -------------------------------------------------------------------------------------
#if ESP32_(2)            // tester
#   define DEBUG  1
#   define BUZZER 23

// ---vvv--- special test modes for ESP32_(2)
//SELECT TESTCASE: goto key_raw2cmd[] of ESP32_(2) -> to directly uncomment 
//#define SUPPORT_MENU_SWITCHING    <- never on ESP32_(2)
// 2026_08_16 fast-connect-retry test: wake on a timer, fire key 1, talk WiFi to the bench AP.
// ESP32_(2) has no LIGHT_SLEEP_TIMEOUT, so every wake is a fresh deep sleep association -
// which is what the retry has to cope with. comment the two lines below to go back to normal
//#define DEBUG_TIMER_WAKEUP 1000   // must also define KEY_BOARD_SIMU_KEY
//#define KEY_BOARD_SIMU_KEY 1
//#define USE_BENCH_WIFI_TARGET
//#define VBAT_DRAIN_SIMU
// ---^^^--- special test modes for ESP32_(2)

//on traditional ESP32 ultra_remotes:
#define VBAT_ADC1_GND_PIN     (gpio_num_t)22
#define VBAT_ADC1_SENSE_PIN   ADC_CHANNEL_0   // GPIO 36 # <- 36(!) == SENSOR_VP
#define VBAT_ADC1_ATTENUATION ADC_ATTEN_DB_6  // use this for 1:1 resistor divider over VBAT (3.3V / 2 == 1650 < 1884mv)

// specials
// experimentally via wireless tag
//#define OTA_SSID ROTA2I_SSID
//#define ESPNOW_CHANNEL ESPNOW_ROTA2I_CHANNEL
//#define ESPNOW_GW_MAC ESPNOW_002_GW_MAC

// -------------------------------------------------------------------------------------
#elif ESP32_(9)          // --- 1 button remote control [ No. 1 ] ---
#   define DEBUG                 1
#   define BUZZER               32

// -------------------------------------------------------------------------------------
#elif ESP32_(10)         // --- 16b button remote control [ No. 2 ] ---
#   define DEBUG                 1
#   define BUZZER                2

#   define SUPPORT_MENU_SWITCHING

// -------------------------------------------------------------------------------------
#elif ESP32_(11)         // --- 4 button remote control [ No. 3 ] ---
#   define DEBUG                 1
#   define BUZZER                4

// -------------------------------------------------------------------------------------
#elif ESP32_(12)         // --- 14b button remote control [ No. 4 ] ---
#   define DEBUG                 1
#   define BUZZER               32

#   define VBAT_ADC1_GND_PIN    (gpio_num_t)22
#   define VBAT_ADC1_SENSE_PIN  ADC_CHANNEL_0     // GPIO 36 # <- 36(!) == SENSOR_VP
#   define VBAT_ADC1_ATTENUATION ADC_ATTEN_DB_6  // use this for 1:1 resistor divider over VBAT (3.3V / 2 == 1650 < 1884mv)
#   define SUPPORT_MENU_SWITCHING

// -------------------------------------------------------------------------------------
#elif ENTITY_15_CLASS
                         // --- 14c/d/e/f/g/h button remote control [ No. 5/6/7/9/10/11 ] ---
                         // --- 16c button remote control [ No. 8 ] ---
#   define DEBUG                 1
#   define BUZZER                2

#   define VBAT_ADC1_GND_PIN    (gpio_num_t)22
#   define VBAT_ADC1_SENSE_PIN  ADC_CHANNEL_0     // GPIO 36 # <- 36(!) == SENSOR_VP
#   define VBAT_ADC1_ATTENUATION ADC_ATTEN_DB_6  // use this for 1:1 resistor divider over VBAT (3.3V / 2 == 1650 < 1884mv)
#   define SUPPORT_MENU_SWITCHING

// -------------------------------------------------------------------------------------
#elif ESP32_(34)         // --- garage 4 button remote control usy1 (urc-nr 13)
#   define DEBUG                 1
#   define BUZZER                2

#define OTA_SSID UFIRE_SSID
#define ESPNOW_CHANNEL ESPNOW_UFIRE_CHANNEL
#define ESPNOW_GW_MAC ESPNOW_HOST14_GW_MAC

// -------------------------------------------------------------------------------------
// --- 4 button (2nd) remote control    
// esp32-36                  aa:bb:cc:00:00:02   ultra_remote_4k/       4 button (2nd) remote control       (urc-nr 14)
// esp32-70                                      ultra_remote_4m/       4 button (3rd) remote control       (urc-nr 16)
// esp32-71                                      ultra_remote_4n/       4 button (4th) remote control       (urc-nr 17)
// esp32-72                                      ultra_remote_4o/       4 button (5th) remote control       (urc-nr 18)
#elif ESP32_(36) || \
      ESP32_(70) || \
      ESP32_(71) || \
      ESP32_(72)
      
#   define DEBUG                 1
#   define BUZZER                2

#   define VBAT_ADC1_GND_PIN    (gpio_num_t)22
#   define VBAT_ADC1_SENSE_PIN  ADC_CHANNEL_0   // GPIO 36 # <- 36(!) == SENSOR_VP
#   define VBAT_ADC1_ATTENUATION ADC_ATTEN_DB_6  // use this for 1:1 resistor divider over VBAT (3.3V / 2 == 1650 < 1884mv)

// -------------------------------------------------------------------------------------
#elif ESP32_(45)         // esp32-45 192.168.0.22                       S3 general tester STAR s3-s-mini ESP32S3FH4R2
#   define DEBUG  1
#   define BUZZER 6
// only one or none!
#   define RGB_GPIO_NUM 48
//#   define LED_GPIO_NUM 48

#define VBAT_ADC1_SENSE_PIN ADC_CHANNEL_2       // is GPIO 3 on S3/ SUPER MINI
#define VBAT_ADC1_ATTENUATION ADC_ATTEN_DB_12   // input range (with 1:1 voltage divider): USB: 2.49, BAT: 1.90
#define SUPPORT_MENU_SWITCHING
#define LIGHT_SLEEP_TIMEOUT 600000000             // 600s resort to deep sleep after this

// S3 SUPERMINI specials
// UPDATE as of 2026_08_31:
// no longer defined/ see comment in ultra_remote_mini for this
//#define ATTENTION_REDUCED_WIFI_POWER 44   // EVEN LOWER DOOR AREA APPEARS TO WORK!! <= STARTING 2026_06_30

/*
 * accu patrol on the tester - same feature as on the ACCU class below, only with an interval you
 * can sit out. MUTUALLY EXCLUSIVE with DEBUG_TIMER_WAKEUP: that one owns the deep sleep timer and
 * replaces the key wakeup entirely, so the two cannot both be armed. hence the comment above -
 * put DEBUG_TIMER_WAKEUP back and the patrol silently disappears, see prepare_keys_for_deep_sleep()
 */
#define VBAT_PATROL_INTERVAL        60      // s, against 5 * 3600 on the real accu devices
#define VBAT_PATROL_CRITICAL      3200      // mV, unloaded. see the ACCU class below

/* 
 *  for [ simplified keyboard connection (single switch) on esp32-45 (STAR) ] use:
 *
 *  pin [4] on the left, pin [3] on the right (counted from top) to gain 2-char cmd [zj] on esp(45)
 *
 *      ----1      1----
 *      ----2      2----
 *      ----3      3---- X-
 *   -X ----4       ----
 *      ----        ----
 */
/*
 * audible-gap bench 2026_09_15: esp32-45 has no hardware keys (automated test board), so the
 * press is simulated. this drives the full process_input() path - chirp, mysend(), status, status
 * tone - once per DEBUG_TIMER_WAKEUP ms, which is the entire path the ear judges.
 *
 * KEY_BOARD_SIMU_KEY is a RAW key 1..16; process_input() then adds dynamic_cmd_offset * 16, so
 * which row of key_raw2cmd[] actually fires depends on the menu the device is left in. read it
 * back off the serial - PR01("espnow cmd: ...") prints the command text itself, so there is no
 * need to work the offset out in advance. with the "2 stationary test" menu selected, raw 16 is
 * the oa/od row (ldoor assert + deassert, non-pre-acked, straight to bell on host3) - the path
 * that bypasses host1 entirely and so tells us whether tcp_server is involved at all.
 *
 * REVERT BOTH BEFORE THIS BOARD GOES BACK TO ANYTHING ELSE: with DEBUG_TIMER_WAKEUP the remote
 * wakes on a timer instead of on its keys, and VBAT_PATROL_INTERVAL silently disappears
 */
/*
 * ENTITY SCOPED ON PURPOSE. this lived at file scope until 2026_09_15, ahead of the entity #if
 * chain, which silently applied it to EVERY ultra_remote entity - and for a remote that can
 * actually reach the cellar, dropping esp32-79 from espnow_gateway_mac[] is a functional loss,
 * not a bench tweak. it belongs to esp32-45 alone: an S3 SUPERMINI cannot reach the cellar, so
 * the entry is pure dead weight here - 32 retries, ~35ms of transmit airtime, never once ACKed
 * in 1033 bursts. measurement above espnow_gateway_mac[] in mcom.h
 */
#define ESPNOW_BENCH_DROP_UNREACHABLE

/*
 * bench defines, stood down 2026_09_15 after the audible-gap campaign. re-arm all four together
 * to drive simulated presses again (esp32-45 has no hardware keys):
 *
 *   DEBUG_TIMER_WAKEUP 1000         ms between simulated presses
 *   DEBUG_TIMER_WAKEUP_KEEP_BUZZER  keep the chirp - it is the 30ms reference being measured
 *   DEBUG_TIMER_WAKEUP_MENU 2       "2 stationary test" (see the note at dynamic_cmd_offset)
 *   KEY_BOARD_SIMU_KEY <row>        09 = ii, 10 = ii ^ (pre-acked), 16 = oa/od via bell on host3
 *
 * NOTE while armed, VBAT_PATROL_INTERVAL above is suppressed - the two share the deep sleep timer
 */
//#define DEBUG_TIMER_WAKEUP 1000
//#define DEBUG_TIMER_WAKEUP_KEEP_BUZZER
//#define DEBUG_TIMER_WAKEUP_MENU 2
/*
 * ROW 12 - "no" + "od", both NOOPs on bell. THE SAFE ROW against the real bell on host3.
 * row 16 pairs "oa" (CMD_ldoor_open_signal_assert) with "od" and ACTUATES the opener on every
 * cycle - do not point a repeating bench at it, see ~/other/bell-test/README
 */
#define KEY_BOARD_SIMU_KEY 12

// -------------------------------------------------------------------------------------
#elif ENTITY_45_CLASS   // ACCU devices, ESP32_(45) IMPLICITLY CAUGHT ABOVE
#   define DEBUG  1
#   define BUZZER 6
#   define RGB_GPIO_NUM 48

#define VBAT_ADC1_SENSE_PIN ADC_CHANNEL_2       // is GPIO 3 on S3/ SUPER MINI
#define VBAT_ADC1_ATTENUATION ADC_ATTEN_DB_12   // input range (with 1:1 voltage divider): USB: 2.49, BAT: 1.90
#define SUPPORT_MENU_SWITCHING
#define LIGHT_SLEEP_TIMEOUT 600000000             // 600s resort to deep sleep after this

// S3 SUPERMINI specials
// UPDATE as of 2026_08_31:
// no longer defined/ see comment in ultra_remote_mini for this
//#define ATTENTION_REDUCED_WIFI_POWER 44   // EVEN LOWER DOOR AREA APPEARS TO WORK!! <= STARTING 2026_06_30

/*
 * these four are the accu (rechargeable LiPo) operated devices: they discharge whether they get
 * used or not, so patrol the cell on the deep sleep timer NEXT TO the keys. a remote left
 * untouched for weeks still gets its chance to report before it deep discharges
 */
#define VBAT_PATROL_INTERVAL  (5 * 3600)    // s
//#define VBAT_PATROL_INTERVAL  10            // 10s/ debug

/*
 * VBAT_PATROL_CRITICAL is a CONSTANT and deliberately not vbat_trigger
 *
 * vbat_trigger is dynamic: the server ratchets it down through the statmsg mysend() hands back,
 * and that is what keeps the regular after-a-cmd report in process_input() from repeating. it
 * works there because that path talks WiFi to KARR_TARGET_HOST, AKA end to end to the real target.
 *
 * the patrol cannot use it. it goes out over ESPNOW, the S3 SUPERMINIs are too weak to cover the
 * area on one gateway, so NUM_ESPNOW_GATEWAYS > 1 and the cmd has to be pre-acked (the trailing
 * " ^", which mcom.h rewrites into "^<n>S ^"). a pre-acked cmd is answered by whichever gateway
 * responds FIRST, not by the final target, so statmsg carries no battery information at all -
 * adapting a threshold from it would be adapting to noise.
 *
 * hence the strategy: fixed threshold, and simply keep reporting every interval until somebody
 * charges the device. once the cell is back above it the alarm stops by itself, with no state to
 * clear - which is the second reason a constant beats a ratchet here.
 *
 * measured UNLOADED (the patrol samples with the radio still down). 3500mV leaves room to still
 * get the alarm out and to charge before the cell takes damage - CHECK THIS AGAINST YOUR CELLS
 *
 *
 * Li-Ion: 4,2V / 3,6V - 3,4V / 2,5 V
 *
 */
#define VBAT_PATROL_CRITICAL      3200      // mV, unloaded
//#define VBAT_PATROL_CRITICAL      8000      // report all/ debug

// -------------------------------------------------------------------------------------
#elif ESP32_(53)        // --- 14i (C5) button remote control [ No. 12 ] ---
#   define DEBUG                 1
#   define BUZZER               26

#   define VBAT_ADC1_GND_PIN    (gpio_num_t)24
#   define VBAT_ADC1_SENSE_PIN  ADC_CHANNEL_1     // GPIO 2 on C5
#   define VBAT_ADC1_ATTENUATION ADC_ATTEN_DB_6  // use this for 1:1 resistor divider over VBAT (3.3V / 2 == 1650 < 1884mv)
#   define SUPPORT_MENU_SWITCHING

// -------------------------------------------------------------------------------------
#else
#   error: no ESP32_x device defined
#endif

#include "mlcf.h"
#ifdef MCFG_LOCAL
#include "mcfg_local.h"
#else
#include "mcfg.h"
#endif
// ===vvv================================= debug timer wakeup ================================vvv===
/*
 * DEBUG_TIMER_WAKEUP: profiling aid. replaces the key press wakeup by a deep sleep timer so the
 * wakeup -> cmd path can be measured without a human sitting on the keys - exactly the trick
 * ultra_remote_mini uses. value is the sleep time in ms.
 *
 * implies BUZZER off, for the same two reasons the mini keeps it off:
 *  1. the LEDC pulls current and time into the middle of what we are measuring
 *  2. ledc_set_freq() aborts on C5 under idf 5.5.5 (ESP_FAIL -> ESP_ERROR_CHECK), see
 *     issue_beep() in mcom.h - so with a buzzer this firmware does not even boot there
 *
 * NEVER leave this enabled in a production build: the remote then wakes on a timer instead of
 * on its keys, and it stays silent
 */
//#define DEBUG_TIMER_WAKEUP 1000       // <== uncomment to profile, ms
/*
 * SKIP_WIRELESS_OFF_BEFORE_DEEP_SLEEP (measured on C5, 2026_08_14)
 *
 * wireless(0) is a bare esp_wifi_stop() on the ESPNOW path and measures 22..42ms, by far the
 * largest item left after the status arrives. it is called "early for energy efficiency", but
 * only ~0.6ms of awake time follows it (wait_for_key_release + prepare_keys_for_deep_sleep),
 * so it saves nothing and burns 22..42ms with the radio still up.
 *
 * deep sleep powers the RF down by itself: esp_phy does
 *   esp_deep_sleep_register_phy_hook(&phy_close_rf)
 * and everything outside RTC memory is gone across the sleep anyway, so there is nothing to leak.
 * only skipped on the deep sleep path - the light sleep path still needs the radio switched off.
 *
 * ESPNOW ONLY (corrected 2026_08_15, measured on entity 53 with ultra_remote_mini).
 *
 * the 2026_08_14 measurement above only ever exercised ESPNOW, but wireless(0) is
 * deinit_WiFi() -> ur_disconnect() when the last tech was WiFi, and there the very same skip is
 * a heavy net LOSS: a WiFi association is state held on the AP, and dropping into deep sleep
 * without disconnecting leaves it dangling for the next wakeup to clean up:
 *
 *      w_link_up     24.5ms  ->  1,661..1,667ms      (six consecutive cycles)
 *      stat            404   ->  2044
 *
 * i.e. 15..20ms of tail saved against 1.64s of association bought. reproducible to a few ms, so
 * it is AP-side state ageing out, not RF. hence the isESPNOW() test at the call site.
 */
#define SKIP_WIRELESS_OFF_BEFORE_DEEP_SLEEP

#if defined(DEBUG_TIMER_WAKEUP)
/*
 * DEBUG_TIMER_WAKEUP_KEEP_BUZZER - keep the buzzer while simulating key presses.
 *
 * the undef below has two reasons (see above) and NEITHER holds for the audible-gap bench:
 *
 *  1. "the LEDC pulls current and time into the middle of what we are measuring" - true when the
 *     thing measured is the wake -> cmd path. here the measurement IS the chirp: its
 *     BEEP_SPIKE_PULSE_WIDTH of 30ms is the entire budget the status tone has to land inside, and
 *     its LEDC runs concurrently with the send by construction. removing it removes the reference
 *     the whole experiment is against, and it is not free either - ultra_remote_mini records the
 *     buzzer lifting init by ~10ms (espnow cmd: 86 -> 96), so a no-buzzer build measures a
 *     different system than the one the ear complained about
 *  2. the ledc_set_freq() abort is C5 only; this is an S3 and mcom.h pins LEDC_USE_APB_CLK for it
 *
 * so it stays OFF by default - every other DEBUG_TIMER_WAKEUP user is profiling the wake path and
 * wants the undef - and the gap bench asks for it explicitly
 */
#if !defined(DEBUG_TIMER_WAKEUP_KEEP_BUZZER)
#   undef BUZZER                        // see above
#endif
#   define DEBUG_TPROF                  // profiling implies the stamps below

// DEBUG_TIMER_WAKEUP requires definition of a key to simulate
// so if not yet defined one define it now
#if !defined(KEY_BOARD_SIMU_KEY)
    #define KEY_BOARD_SIMU_KEY 1    // use a harmless value to fit in any of the given arrays
#endif
// key board simulation/ programmed 2023/05/27 in Spain/ Andalusia/ Mijas
#define SIMULATE_KEY_PRESSES_WITH(_key) \
{ \
    static _i8 was_here; \
    if (bootCount == 1) ++bootCount; \
    key = !was_here ? _key : 0; \
    ++was_here; \
}
#endif
// ===^^^================================= debug timer wakeup ================================^^^===

/*
 * ESPNOW_RESEND_UNANSWERED (see mcom.h): esp32-46 loses the FIRST cmd after a wake - its
 * transmissions in the first ~100ms after the radio starts are mostly undecodable, by gateways
 * and sniffers alike. soak 2026_09_29: first cmd unanswered 52 of 703 wakes, second cmd 9 of 671,
 * and in every one of those wakes that had a second cmd it got through, from the same spot 1s
 * later. every other remote ~100%. so one repeat, with the same serial, costs 1s only when needed
 */
#if ESP32_(46)
#define ESPNOW_RESEND_UNANSWERED 1
#endif

#include "mcom.h"

// ===vvv================================= wakeup path profiling =============================vvv===
/*
 * same scheme as ultra_remote_mini: stamps collected in RAM, dumped right before deep sleep, so
 * the marks don't perturb what they measure. us resolution because tstamp() is 10ms granular
 */
#if !defined(DEBUG_TPROF)

/*
 * production: no stamps, no dump. the dump alone is ~8 lines per keypress and printing blocks
 * at line rate, so it would cost awake time (AKA battery) on every single key
 */
#define TPROF(nam)
#define TPROF_DUMP()

#else   // !defined(DEBUG_TPROF)

#define TPROF_MAX 24

_u32 tprof_time[TPROF_MAX];
_i8cp tprof_name[TPROF_MAX];
_u8  tprof_cnt;

#define TPROF(nam) \
do { \
    if (tprof_cnt < TPROF_MAX) { \
        tprof_name[tprof_cnt] = nam; \
        tprof_time[tprof_cnt++] = (_u32)esp_timer_get_time(); \
    } \
} while (0)

#define TPROF_DUMP() \
do { \
    for (_u8 i = 0; i < tprof_cnt; ++i) { \
        PR00("TPROF %-20s %8lu %+8ld\n", \
            tprof_name[i], \
            (unsigned long)tprof_time[i], \
            (long)(tprof_time[i] - tprof_time[i ? i - 1 : 0]) \
        ); \
    } \
} while (0)

#endif  // !defined(DEBUG_TPROF)
// ===^^^================================= wakeup path profiling =============================^^^===
// ---^^^--- standard includes ---^^^--- 

/* ---vvv--- switched deep/ light sleep support ---vvv--- */
#if defined(LIGHT_SLEEP_TIMEOUT)

/*
 * consider all dynamic_access_target variants here:
 * currently only AT_ESPNOW_to_PROX implies deep sleep
 * ATTENTION: take care where and when in code dynamic changes to dynamic_access_target are applied
 *            as this has massive impact on overall chip init
 */
// TARGETED sleep mode
#define MY_SLEEP_MODE_IS_DEEP() (dynamic_access_target == AT_ESPNOW_to_PROX)

// CURRENTLY ACTIVE sleep mode
_i32 my_sleep_mode_is_deep;

#endif  // defined(LIGHT_SLEEP_TIMEOUT)

/*
 * is the wireless(0) we are about to do a PRE-DEEP-SLEEP one? entities without LIGHT_SLEEP_TIMEOUT
 * only ever deep sleep, so there the answer is always yes
 */
#if defined(LIGHT_SLEEP_TIMEOUT)
#define WIRELESS_OFF_IS_BEFORE_DEEP_SLEEP() (my_sleep_mode_is_deep)
#else
#define WIRELESS_OFF_IS_BEFORE_DEEP_SLEEP() 1
#endif

/* ---^^^--- switched deep/ light sleep support ---^^^--- */

/* ---vvv--- LED/RGB section ---vvv--- */
#if defined(RGB_GPIO_NUM)
    #define rgb_red(on)   ws2812_set(on, 0, 0)
    #define rgb_green(on) ws2812_set(0, on, 0)
    #define rgb_blue(on)  ws2812_set(0, 0, on)
    #define rgb_off()     ws2812_set(0, 0, 0)
#else
    // make it all void
    #define rgb_red(a)
    #define rgb_green(a)
    #define rgb_blue(a)
    #define rgb_off()
#endif

#if defined(LED_GPIO_NUM)
    #define led_act(on)  ledctl(LED_GPIO_NUM, on, ACT_HGH)       // high active
#else
    // make it all void
    #define led_act(on) 
#endif
/* ---^^^--- LED/RGB section ---^^^--- */

/* ---vvv--- access target multiplexer ---vvv--- */
#define AT_NOT_IN_USE                   255
#define _AT_ROTA2G_to_RPI5    AT_NOT_IN_USE //  0 // retired standard until 2016_03_02
#define AT_TETHER_to_KARRp                1 // mobile: effectively point to identical data but provide a different indx 
#define AT_TETHER_to_KARRd                2 // mobile: effectively point to identical data but provide a different indx 
#define _AT_ROTA2G_to_RPID    AT_NOT_IN_USE //  3
#define _AT_ROTA2G_to_ROS2    AT_NOT_IN_USE //  4        
#define _AT_FASTACCPT_to_RPI5 AT_NOT_IN_USE //  5 // performance testing only 
#define _AT_FASTACCPT_to_RPID AT_NOT_IN_USE //  6 // performance testing only 
#define AT_ESPNOW_to_PROX                 7 // home: standard as of 2016_03_02
#define _AT_FASTACCPT_to_HOST2 AT_NOT_IN_USE //  8 // performance testing only

typedef struct {
    _u8  accp;      // indx into accpts[] array
    _i8cp host;     // target host in ascii
    _u16 port;
} access_target_str;

/*
 * collect all possible SSID, HOST, TARGET combinations in one array
 * to make them accessible via an single number, saved in ultra remote hardware
 */
access_target_str
access_target[] = {
    { 0          , RPI5_TARGET_HOST,    RPI5_TARGET_PORT },     // [  0 ]            static access to RPI5_TARGET via ROTA2G
    { TETHER_SSID, KARR_TARGET_HOST,    KARR_TARGET_PORT },     // [  1 ] [ in use ] static access to KARR_TARGET via TETHER   (player version)
    { TETHER_SSID, KARR_TARGET_HOST,    KARR_TARGET_PORT },     // [  2 ] [ in use ] static access to KARR_TARGET via TETHER   (door version)
    { 0          , RPID_TARGET_HOST,    RPID_TARGET_PORT },     // [  3 ]            static access to RPID_TARGET via ROTA2G  
    { 0          , ROS2_TARGET_HOST,    ROS2_TARGET_PORT },     // [  4 ]            static access to ROS2_TARGET via ROTA2G  

    // for performance testing use wireless tag locally in AZI-T
    { ROTA2I_SSID, RPI5_TARGET_HOST,    RPI5_TARGET_PORT },     // [  5 ]            static access to RPI5_TARGET via FASTACCPT
    { ROTA2I_SSID, RPID_TARGET_HOST,    RPID_TARGET_PORT },     // [  6 ]            static access to RPID_TARGET via FASTACCPT
    { ESPNOW_SSID, ESPNOW_TARGET_HOST,  ESPNOW_TARGET_PORT },   // [  7 ] [ in use ] static access to PROXY via ESPNOW
    { ROTA2I_SSID, "host2.example.com",  RPID_TARGET_PORT },     // [  8 ]            static access to host2 via FASTACCPT
};
/* ---^^^--- access target multiplexer ---^^^--- */

/* ---vvv--- KEY section ---vvv--- */

// prefix for menu selection commands
#define M_PFX "2" 

typedef struct {
    _i8cp cmd;  // opening ascii cmd/menu code (mostly implies auto close)
    _i8cp cmdClosing;  // closing ascii cmd/menu code (if not auto closed)
} raw2cmd_str;

typedef struct {
    _u8  access_target_index;    // indx into access_target[] array
    _u8  off;   // indx offset into alternative cmd block
} raw2menu_str;

#define MENU_SWITCH_INTRO_TIMEOUT   1000    // time in ms to initially detect a menu switching activity 
#define MENU_SWITCH_TARGET_TIMEOUT  5000    // max. time in ms to wait for getting the target menu (if not indicated earlier)
#define MENU_SWITCH_RESCAN_SPACING    10    // defines the rescan rate to detect KEY_META press activity
#define KEY_SCAN_SETTLE_US           100    // settle time in us between driving a SNS line low and reading the HOT lines
#define MENU_SWITCH_ALLOW_DEBOUNCE    20    // in the hope signal stabilizes after that amount of time in ms

//
// list of interpreted key codes goes here
//  (we skipped k + l to gain similar key codes on both 14 + 16 buttons URs)
//
#define KEY_CODE_MOBL_PLAY_MENU 0x07                // MENU key '2m'
#define KEY_CODE_MOBL_DOOR_MENU 0x06                // MENU key '2o'
#define KEY_CODE_META 0x02                          // META key '2p'

#define CMD_KEY_OFFSET_BASE 1
#define CMD_KEY_OFFSET_MULTIPLIER 16
#define IS_MENU_KEY(key) ((key) <= CMD_KEY_OFFSET_MULTIPLIER)

//
// unfortunately UR-control must retain some minimal state after menu switch to
//  - know what access point to contact
//  - which host/IP/port to address
//  - know special behavior specific to particular menues (aka long-press allowed, menu-toggles et.al.)
//
#if ESP32_(4555555555) 
/*
 * debug light sleep
 */
RTC_DATA_ATTR _u8 dynamic_access_target = AT_TETHER_to_KARRp;   // see [ simplified keyboard connection (single switch) on esp32-45 (STAR) ] above
#else
#if defined(USE_BENCH_WIFI_TARGET)
/*
 * ESP32_(2) has no SUPPORT_MENU_SWITCHING, so key_raw2menu[] is never consulted and
 * _dynamic_access_target just inherits this default - it has to point at the bench AP itself
 */
RTC_DATA_ATTR _u8 dynamic_access_target = AT_TETHER_to_KARRp;   // 2026_08_25 test device
#else
RTC_DATA_ATTR _u8 dynamic_access_target = AT_ESPNOW_to_PROX;    // current menu selected (may not be zero)
#endif
#endif
/*
 * DEBUG_TIMER_WAKEUP_MENU - preselect the cmd menu for a simulated-press bench.
 *
 * dynamic_cmd_offset is RTC_DATA_ATTR, so it survives deep sleep but comes back as
 * CMD_KEY_OFFSET_BASE (1) on the hard reset a flash ends with - and nothing in a
 * DEBUG_TIMER_WAKEUP build ever presses a menu key to move it. so raw key 16 lands on menu 1
 * ("1 stationary player", cmd "zh") rather than menu 2 ("2 stationary test"), where the
 * ii / ii ^ / oa+od rows live.
 *
 * pairing offset 2 with the AT_ESPNOW_to_PROX default above is NOT an invented combination:
 * key_raw2menu[] entry "12 b" is exactly { AT_ESPNOW_to_PROX, 2 }, i.e. what pressing that menu
 * key by hand would set. keep the two consistent if this is ever pointed at another menu
 */
#if defined(DEBUG_TIMER_WAKEUP_MENU)
RTC_DATA_ATTR _u8 dynamic_cmd_offset = DEBUG_TIMER_WAKEUP_MENU;
#else
RTC_DATA_ATTR _u8 dynamic_cmd_offset = CMD_KEY_OFFSET_BASE;     // current cmd offset coming along with selected menu
                                                                // offsets into first excepted menu since 2026_09_10
#endif
              _u8 _dynamic_access_target;                       // preliminary version of the above (until acknowledged by the server)

#include "hal/gpio_ll.h"    // required for GPIO.in, GPIO.in1.val...
#include "esp_rom_sys.h"    // required for esp_rom_delay_us() in scan_keys()

_u8
_ffs(_u16 v)
{
    _u8 r = 0;

    if (!v)         { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;
    if (!(v >>= 1)) { goto out; } ++r;

out:
    return r;
}

/*
* Bit Twiddling Hacks
https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetKernighan

Counting bits set, Brian Kernighan's way

Published in 1988, the C Programming Language 2nd Ed. (by Brian W. Kernighan and Dennis M. Ritchie)
mentions this in exercise 2-9. On April 19, 2006 Don Knuth pointed out to me that this method 
"was first published by Peter Wegner in CACM 3 (1960), 322. (Also discovered independently 
by Derrick Lehmer and published in 1964 in a book edited by Beckenbach.)"
*/
_u32
count_bits(_u32 v) {
    _u32 c;

    for (c = 0; v; c++)
    {
      v &= v - 1;
    }
    return c;
}

// -------------------------------------------------------------------------------------
#if ESP32_(2)                          // --- general purpose tester currently: 1 button remote control ---

#define KEY_SNS (gpio_num_t)2
#define KEY_SNS_MASK (1 << KEY_SNS)

// first non encommented will execute
raw2cmd_str 
key_raw2cmd[] = {
    { },                                                        // 00                 place holder (no key)
//  { "@beep= f:1000 c:1 t:.05 p:.25 g:-20 ^host2.example.com:8888", 0 },
    { "@beep= f:1000 c:1 t:.05 p:.25 g:-20 ^", 0 }, 
//  { "no", 0 },                                           // 2026_08_16 fast connect retry test
//  { "zp", 0 },                                           // pause // ### needs espdoor *NOT* running prior to use ###
//  { "no", 0 },                                           // direct host2 access, needs bell running on S3 SUPER MINI: 
                                                                // wifi/eth cmd: 73 no -> [host2.example.com:8899]
                                                                // stat: 113 #[XX]#[0]#[0]#[xxx]#[0]#[0]              !!!
    //
    // cmd over pq-system (requires port 8899)
    //
    // pq-ascii    pq-code:    bell-pipe:  req key release:
    //
    // I_D_OPUL    mr          ya          yes     
    // I_D_CLU     mc          dc         
    // I_D_OPL     my          oa          yes       
    // I_D_OPU     mp          do     
    // release     ic          od
    //
//  { "oa", "od" }, // ldoor open signal assert 

    //
    // cmd direct access (requires port 8899)
    //
//  { "no", 0 }, // 
//  { "np", 0 }, // 
//  { "nq", 0 }, // 
//  { "kp", 0 }, // dual open ldoor + udoor (armed open variant) // ### needs espdoor running prior to use ###

    //
    // running into menu change items will crash cause no such entries are defined for ESP32_(2)
    // -> long key press disabled below for ESP32_(2)
    //
};

raw2menu_str  
key_raw2menu[] = {
    { },  
    { AT_ESPNOW_to_PROX,        0 },
//  { AT_ROTA2C_to_HOST2,        0 },                            // 2026_08_16 fast connect retry test
//  { AT_ESPNOW_to_PROX,        0 },
//  { _AT_FASTACCPT_to_RPI5,    0 },
//  { AT_TETHER_to_KARRp,       0 },
//  { _AT_FASTACCPT_to_HOST2,    0 },
//  { _AT_FASTACCPT_to_RPID,    0 },
//  { _AT_FASTACCPT_to_RPID,    0 },
//  { _AT_FASTACCPT_to_RPID,    0 },
//  { _AT_FASTACCPT_to_RPID,    0 },
//  { AT_TETHER_to_KARRp,       0 },
};

void
init_keys()
{
TP05
    gpio_config_t io_conf = {       // pulls and intrs are auto. set off
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1 << KEY_SNS,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

_u32
scan_keys()
{
TP05
    _u32 key = gpio_get_level(KEY_SNS);  // high if pressed

#ifdef DEBUG_TIMER_WAKEUP
    SIMULATE_KEY_PRESSES_WITH(KEY_BOARD_SIMU_KEY);
#endif

    return key;   // all key presses (if any) must fit (map) entirely into key_raw2cmd[]
}

// -------------------------------------------------------------------------------------
#elif ESP32_(9)                      // --- 1 button remote control ---

// single sense key with internal pullup in use
// must be connected to a RTC capable terminal to allow use of esp_sleep_enable_ext0_wakeup() 
#define KEY_SNS (gpio_num_t)4

raw2cmd_str 
key_raw2cmd[] = {
    { },                                            // 00                 place holder (no key)
    { "oa ^host3.example.com:8899", "od ^host3.example.com:8899" },    // espnow cmd: 145 [oa ^host3.example.com:8899] len 21 -> gateway
};

void
init_keys()
{
TP05
    gpio_config_t io_conf = {       // pulls and intrs are auto. set off
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1 << KEY_SNS,
        .pull_down_en = 0,
        .pull_up_en = 1,  // experimental: use the builtin pull as it appears to have no impact on deep sleep current
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

_u32
scan_keys()
{
TP05
    _u32 key = !gpio_get_level(KEY_SNS);  // low if key pressed

#ifdef DEBUG_TIMER_WAKEUP
    SIMULATE_KEY_PRESSES_WITH(KEY_BOARD_SIMU_KEY);
#endif

    return key;   // all key presses (if any) must fit (map) entirely into key_raw2cmd[]
}

// -------------------------------------------------------------------------------------
#elif ESP32_(11) || ESP32_(34) || ESP32_(36) || ESP32_(70) || ESP32_(71) || ESP32_(72)      // --- 4 button remote control ---

// ESP32_(11):
// 4 sense keys pulled down by external 470k
// must be connected to RTC capable terminals to allow ESP_EXT1_WAKEUP_ANY_HIGH

// ESP32_(34):
// ESP32_(36):
// uses internal RTC pull downs

#if ESP32_(11)
#define KEY_SNS_0 26
#define KEY_SNS_1 27
#define KEY_SNS_2 14
#define KEY_SNS_3 12
#else   // ESP32_(34) || ESP32_(36) || ESP32_(70) || ESP32_(71) || ESP32_(72)
#define KEY_SNS_0 33    // 32 bit OVERFLOW ATTENTION!
#define KEY_SNS_1 27
#define KEY_SNS_2 26
#define KEY_SNS_3 25
#endif

#define KEY_SNS_MASK ((_u64)1 << KEY_SNS_0 | \
                            1 << KEY_SNS_1 | \
                            1 << KEY_SNS_2 | \
                            1 << KEY_SNS_3)
raw2cmd_str 
key_raw2cmd[] = {
#if ESP32_(11) || ESP32_(70)
    { },                                                                        // . . . .    00 na                   
    { "oa ^host3.example.com:8899", "od ^host3.example.com:8899" },                                // . . . 1    01 (ldoor open)
    { "@beep= f:1000 c:1 t:.05 p:.25 g:-20 ^", 0 },                        // . . 1 .    02 (debug beep)                           
    { },                                                                        // . . 1 1    03 na                   
    { "vu ^", 0 },                                                         // . 1 . .    04 (/home/toh/bin/access_door.breaker)
    { },                                                                        // . 1 . 1    05 na                   
    { },                                                                        // . 1 1 .    06 na                   
    { },                                                                        // . 1 1 1    07 na                   
    { "vy ^", 0 },                                                         // 1 . . .    08 (/home/toh/bin/access_door leave)
    { },                                                                        // 1 . . 1    09 na                   
    { },                                                                        // 1 . 1 .    10 na                   
    { },                                                                        // 1 . 1 1    11 na                   
    { },                                                                        // 1 1 . .    12 na                   
    { },                                                                        // 1 1 . 1    13 na                   
    { },                                                                        // 1 1 1 .    14 na                   
    { },                                                                        // 1 1 1 1    15 na                   
#elif ESP32_(34)                // usys garage
    { },                                                                        // . . . .    00 na                   
    { "@beep=garage_toggle0 ^", 0 },                                       // . . . 1    01
    { "@beep=garage_toggle1 ^", 0 },                                       // . . 1 .    02
    { },                                                                        // . . 1 1    03 na                   
    { "@beep=garage_toggle2 ^", 0 },                                       // . 1 . .    04 
    { },                                                                        // . 1 . 1    05 na                   
    { },                                                                        // . 1 1 .    06 na                   
    { },                                                                        // . 1 1 1    07 na                   
    { "@beep=garage_toggle3 ^", 0 },                                       // 1 . . .    08
    { },                                                                        // 1 . . 1    09 na                   
    { },                                                                        // 1 . 1 .    10 na                   
    { },                                                                        // 1 . 1 1    11 na                   
    { },                                                                        // 1 1 . .    12 na                   
    { },                                                                        // 1 1 . 1    13 na                   
    { },                                                                        // 1 1 1 .    14 na                   
    { },                                                                        // 1 1 1 1    15 na                   
#else
    { },                                                                        // . . . .    00 na                   
    { "@beep= f:1000 c:1 t:.05 p:.25 g:-20 ^", 0 },                        // . . . 1    01
    { "@beep= f:1101 c:1 t:.05 p:.25 g:-20 ^", 0 },                        // . . 1 .    02
    { },                                                                        // . . 1 1    03 na                   
    { "@beep= f:1202 c:1 t:.05 p:.25 g:-20 ^", 0 },                        // . 1 . .    04 
    { },                                                                        // . 1 . 1    05 na                   
    { },                                                                        // . 1 1 .    06 na                   
    { },                                                                        // . 1 1 1    07 na                   
    { "@beep= f:1303 c:1 t:.05 p:.25 g:-20 ^", 0 },                        // 1 . . .    08
    { },                                                                        // 1 . . 1    09 na                   
    { },                                                                        // 1 . 1 .    10 na                   
    { },                                                                        // 1 . 1 1    11 na                   
    { },                                                                        // 1 1 . .    12 na                   
    { },                                                                        // 1 1 . 1    13 na                   
    { },                                                                        // 1 1 1 .    14 na                   
    { },                                                                        // 1 1 1 1    15 na                   
#endif
};

void
init_keys()
{
TP05
#if DEBUG > 5
    if (!esp_sleep_is_valid_wakeup_gpio(KEY_SNS_0)) {
        PR00("can't use %d as wakeup source\n", KEY_SNS_0);
        vTaskDelay(pdMS_TO_TICKS(3600000000));      // try to wait before panic
        ESP_ERROR_CHECK(ESP_FAIL);  
    }
    if (!esp_sleep_is_valid_wakeup_gpio(KEY_SNS_1)) {
        PR00("can't use %d as wakeup source\n", KEY_SNS_1);
        vTaskDelay(pdMS_TO_TICKS(3600000000));      // try to wait before panic
        ESP_ERROR_CHECK(ESP_FAIL);  
    }
    if (!esp_sleep_is_valid_wakeup_gpio(KEY_SNS_2)) {
        PR00("can't use %d as wakeup source\n", KEY_SNS_2);
        vTaskDelay(pdMS_TO_TICKS(3600000000));      // try to wait before panic
        ESP_ERROR_CHECK(ESP_FAIL);  
    }
    if (!esp_sleep_is_valid_wakeup_gpio(KEY_SNS_3)) {
        PR00("can't use %d as wakeup source\n", KEY_SNS_3);
        vTaskDelay(pdMS_TO_TICKS(3600000000));      // try to wait before panic
        ESP_ERROR_CHECK(ESP_FAIL);  
    }
#endif
    gpio_config_t io_conf = {       // pulls and intrs are auto. set off
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = KEY_SNS_MASK,
        .pull_down_en = 1,
        .pull_up_en = 0,            
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

_u32
scan_keys()
{
TP05
    _u32 key = 0;

    key = key << 1 | gpio_get_level(KEY_SNS_3);
    key = key << 1 | gpio_get_level(KEY_SNS_2);
    key = key << 1 | gpio_get_level(KEY_SNS_1);
    key = key << 1 | gpio_get_level(KEY_SNS_0);

#ifdef DEBUG_TIMER_WAKEUP
    SIMULATE_KEY_PRESSES_WITH(KEY_BOARD_SIMU_KEY);
#endif

    return key;  // all key presses (if any) must fit (map) entirely into key_raw2cmd[]
}

// -------------------------------------------------------------------------------------
#elif ESP32_(10) || \
      ESP32_(12) || \
      ENTITY_15_CLASS || \
      ENTITY_45_CLASS || \
      ESP32_(53) 

#if ENTITY_45_CLASS  // --- S3 SUPERMINI

#define KEY_HOT_0 10
#define KEY_HOT_1 11
#define KEY_HOT_2 12
#define KEY_HOT_3 13

#define KEY_SNS_0  1
#define KEY_SNS_1  2
#define KEY_SNS_2  4
#define KEY_SNS_3  5

#elif ESP32_(53)                     // --- 14i button remote control c5 ---

// hot (feeding) keys pulled up by external 7.5k 
#define KEY_HOT_0  7
#define KEY_HOT_1  8
#define KEY_HOT_2 10
#define KEY_HOT_3  9

// sense keys pulled down by external 47k
// must be connected to RTC capable terminals to allow ESP_EXT1_WAKEUP_ANY_HIGH
#define KEY_SNS_0  3
#define KEY_SNS_1  0
#define KEY_SNS_2  1
#define KEY_SNS_3  6

#elif ESP32_(12)                       // --- 14b button remote control ---

// hot (feeding) keys pulled up by external 51k (UPDATE: must additionally use internal PULL for reliable double key press detection)
#define KEY_HOT_0 17  
#define KEY_HOT_1 16    
#define KEY_HOT_2 25    // KEY_SNS_0 25 on ESP32_10
#define KEY_HOT_3  5    // (STRAP, internal pull up)

// sense keys pulled down by external 100k
// must be connected to RTC capable terminals to allow ESP_EXT1_WAKEUP_ANY_HIGH
#define KEY_SNS_0 27    // KEY_SNS_2 27 on ESP32_10
#define KEY_SNS_1 26    // KEY_SNS_1 26 on ESP32_10
#define KEY_SNS_2  4    // KEY_HOT_2  4 on ESP32_10 (STRAP, internal pull down)
#define KEY_SNS_3 14    // KEY_SNS_3 14 on ESP32_10

#elif ESP32_(10)                     // --- 16b button remote control ---

// sense keys connect to feeders on cross points
// when the corresponding key is depressed.
// causing a 'high' level being detected to wakeup.
// after this the matrix is scanned.
// feeders can optionally use RTC capable terminals (but with no benefit)

//
// ATTENTION: strapping conflicts
//
// GPIO12 (KEY_HOT_3) is sampled during reset. In case of a high level resulting in 
// a boot:0x33 undefined behavior results. E.g. system Crashes often.
// Though we externally pull up with 
//   51k the pin samples: low level.
//   30k the pin samples: low level
//   20k the pin samples: high level    ERROR!
// => so never use lower ohm values for KEY_HOT_3 pull up!
//
// further more connecting GPIO12 to GPIO14 (through key press)
// mostly results in boot:0x33. This is because GPIO14 is pulled high internally.
//

// hot (feeding) keys pulled up by external 51k 
#define KEY_HOT_0 33    // 32 bit OVERFLOW ATTENTION!
#define KEY_HOT_1 32    // 32 bit OVERFLOW ATTENTION!
#define KEY_HOT_2  4    // KEY_SNS_2  4 on ESP32_12 (STRAP, internal pull down)
#define KEY_HOT_3 12    // (STRAP, internal pull down)

// sense keys pulled down by external 100k 
// must be connected to RTC capable terminals to allow ESP_EXT1_WAKEUP_ANY_HIGH
#define KEY_SNS_0 25    // KEY_HOT_3 25 on ESP32_12
#define KEY_SNS_1 26    // KEY_SNS_1 26 on ESP32_12
#define KEY_SNS_2 27    // KEY_SNS_0 27 on ESP32_12
#define KEY_SNS_3 14    // KEY_SNS_3 14 on ESP32_12

#else                                       // --- 14c/d/e/f/g/h button remote control ---
                                            // --- 16c button remote control ---

// hot (feeding) keys pulled up by external 51k (UPDATE: must additionally use internal PULL for reliable double key press detection)
#define KEY_HOT_0 16
#define KEY_HOT_1 17
#define KEY_HOT_2 18
#define KEY_HOT_3 19

// sense keys pulled down by external 100k
// must be connected to RTC capable terminals to allow ESP_EXT1_WAKEUP_ANY_HIGH
#define KEY_SNS_0 33    // 32 bit OVERFLOW ATTENTION!
#define KEY_SNS_1 25 
#define KEY_SNS_2 26 
#define KEY_SNS_3 27 

#endif

#define KEY_SNS_MASK ((_u64)1 << KEY_SNS_0 | \
                            1 << KEY_SNS_1 | \
                            1 << KEY_SNS_2 | \
                            1 << KEY_SNS_3)

#define KEY_HOT_MASK ((_u64)1 << KEY_HOT_0 | \
                      (_u64)1 << KEY_HOT_1 | \
                            1 << KEY_HOT_2 | \
                            1 << KEY_HOT_3)
/*
 * ffs-scanned key codes to key caps ordering:
 */
#if 0
------------------ key codes for ancient ESP32_(10) ultra_remotes ------------------

native          mapped                                          physical botton in hardware order:
raw             to:
code:

16  15          2a 2b                                           10000000 00000000   01000000 00000000
12  11          2c 2d                                           00001000 00000000   00000100 00000000
8    7          2e 2f                                           00000000 10000000   00000000 01000000
4    3  =>      2g 2h                                           00000000 00001000   00000000 00000100
13  14          2i 2j                                           00010000 00000000   00100000 00000000
9   10]        [2k-2l]  (do not exist on 14 button controls)    00000001 00000000   00000010 00000000
5    6          2m 2n                                           00000000 00010000   00000000 00100000
1    2          2o 2p                                           00000000 00000001   00000000 00000010

32  31          2A 2B   
28  27          2C 2D   
24  23          2E 2F   
20  19  =>      2G 2H   
29  30          2I 2J   
25  26]        [2K 2L]  (do not exist on 14 button controls)
21  22          2M 2N   
17  18          2O 2P   

------------------ key codes for NON - ESP32_(10) ultra_remotes ------------------

native          mapped                                          physical botton in hardware order:
raw             to:
code:                 

16  12          2a 2b                                           1000000000000000 0000100000000000
15  11          2c 2d                                           0100000000000000 0000010000000000
14  10          2e 2f                                           0010000000000000 0000001000000000
13   9  =>      2g 2h                                           0001000000000000 0000000100000000
8    4          2i 2j                                           0000000010000000 0000000000001000
7    3          2m 2n                                           0000000001000000 0000000000000100
6    2          2o 2p                                           0000000000100000 0000000000000010
5    1]        [2k 2l]  (do not exist on 14 button controls)    0000000000010000 0000000000000001

32  28          2A 2B   
31  27          2C 2D   
30  26          2E 2F   
29  25  =>      2G 2H   
24  20          2I 2J   
23  19          2M 2N   
22  18          2O 2P   
21  17]        [2K 2L]  (do not exist on 14 button controls)

#endif

#if ESP32_(10)  // esp32-10 aa:bb:cc:00:00:03 ultra_remote_16b/ multi key remote control (urc-nr 2) the mother of all ultra_remotes

// make ancient ESP32_(10) compatible to the rest of the world
// translate native raw key codes from ESP32_(10) to non-ESP32_(10) (standard) order

_u8 tr_keys[] = {
 0, // 0
 6, // 1
 2, // 2
 9, // 3
13, // 4
 7, // 5
 3, // 6
10, // 7
14, // 8
 5, // 9
 1, // 10
11, // 11
15, // 12
 8, // 13
 4, // 14
12, // 15
16, // 16
};

#endif  // ESP32_(10)

/*
 * 16  12          2a 2b                                      
 * 15  11          2c 2d                                     
 * 14  10          2e 2f                                    
 * 13   9    =>    2g 2h                                   
 * 8    4          2i 2j                                  
 *[5    1]        [2k 2l]  (do not exist on 14 button controls)
 * 7    3          2m 2n                                 
 * 6    2          2o 2p                                
 *
 * ------------------------------------------------
 * | 1 stationary player   |   2 stationary test  |
 * | 3 stationary audio    |   .                  |
 * | 4 stationary light    |   .                  |
 * | 5 stationary 001      |   .                  |
 * | .                     |   .                  |
 * | 6 mobile player       |   7 mobile access    |
 * | 8 mobile door         |   .                  |
 * ------------------------------------------------
 *
 * policy:
 *  use '^' immediate flag only where needed e.g. for long 
 *  running commands that would timeout aggressive u-remote
 *  timing otherwise
 *
 */
raw2cmd_str  
key_raw2cmd[] = {

    { }, 

    // do not pre ack to get at the real server response 
// --- p-command p-menu selection ---   
    {       ".", 0 }, // 01 l  does not exist on 14 button
    { M_PFX "P", 0 }, // 02 p                 
    { M_PFX "N", 0 }, // 03 n                 
    { M_PFX "J", 0 }, // 04 j                 

    {       ".", 0 }, // 05 k  does not exist on 14 button
    { M_PFX "O", 0 }, // 06 o                 
    { M_PFX "M", 0 }, // 07 m                 
    { M_PFX "I", 0 }, // 08 i                 

    { M_PFX "H", 0 }, // 09 h                 
    { M_PFX "F", 0 }, // 10 f                 
    { M_PFX "D", 0 }, // 11 d                 
    { M_PFX "B", 0 }, // 12 b                 

    { M_PFX "G", 0 }, // 13 g                 
    { M_PFX "E", 0 }, // 14 e                 
    { M_PFX "C", 0 }, // 15 c                 
    { M_PFX "A", 0 }, // 16 a                 

    // deliberately not pre-acked ATM 
// --- 1 stationary player ---
    { "."                          , 0                           },  // 01 l*     does not exist on 14 button
    { "zz"                         , 0                           },  // 02 p      
    { "zj"                         , 0                           },  // 03 n      
    { "zr"                         , 0                           },  // 04 j                                               

    { "."                          , 0                           },  // 05 k*     does not exist on 14 button
    { "zv"                         , 0                           },  // 06 o      
    { "zl"                         , 0                           },  // 07 m      
    { "zt"                         , 0                           },  // 08 i                                               

    { "zx"                         , 0                           },  // 09 h                                                
    { "zn"                         , 0                           },  // 10 f                                                                      
    { "zm"                         , 0                           },  // 11 d      
    { "zk"                         , 0                           },  // 12 b      

    { "zc"                         , 0                           },  // 13 g                                                
    { "zb"                         , 0                           },  // 14 e                                                   
    { "zp"                         , 0                           },  // 15 c      
    { "zh"                         , 0                           },  // 16 a      

    // deliberately not pre-acked ATM 
// --- 2 stationary test -----
    { "."                          , 0                           },  // 01 l*     does not exist on 14 button
    { "."                          , 0                           },  // 02 p      KEY_CODE_META
    { "fc"                         , 0                           },  // 03 n      test sleep non pre-acked
    { "fc ^"                       , 0                           },  // 04 j      test sleep pre-acked

    { "."                          , 0                           },  // 05 k*     does not exist on 14 button
    { "."                          , 0                           },  // 06 o      KEY_CODE_MOBL_DOOR_MENU
    { "dc ^host3.example.com:8899"      , 0                           },  // 07 m      KEY_CODE_MOBL_PLAY_MENU
    { "."                          , 0                           },  // 08 i

    { "ii"                         , "ii"                   },  // 09 h      test routed implicitly to STD_TARGET non pre-acked
    { "ii ^"                       , "ii ^"                 },  // 10 f      test routed implicitly to STD_TARGET pre-acked
    { "ii ^host2.example.com:8888"       , "ii ^host2.example.com:8888" },  // 11 d      test routed explicitly to host2/ ATTENTION p-proc needs workaround in gw
    { "no ^host3.example.com:8899"      , "od ^host3.example.com:8899"},  // 12 b      test NOOP + ldoor open signal deassert

    { "."                          , 0                           },  // 13 g
    { "."                          , 0                           },  // 14 e
    { "da ^host3.example.com:8899"      , 0                           },  // 15 c
    { "oa ^host3.example.com:8899"      , "od ^host3.example.com:8899"},  // 16 a      ldoor open signal assert + ldoor open signal deassert

// --- 3 stationary audio ----
    { "."                          , 0                           },  // 01 l*     does not exist on 14 button
    { "zp;xx;cc ^"                 , 0                           },  // 02 p      
    { "."                          , 0                           },  // 03 n      
    { "."                          , 0                           },  // 04 j                                               

    { "."                          , 0                           },  // 05 k*     does not exist on 14 button
    { "xc;cz ^"                    , 0                           },  // 06 o      
    { "."                          , 0                           },  // 07 m      
    { "cb ^"                       , 0                           },  // 08 i                                               

    { "."                          , 0                           },  // 09 h  
    { "xc ^"                       , 0                           },  // 10 f                                                                      
    { "xx ^"                       , 0                           },  // 11 d      
    { "xz ^"                       , 0                           },  // 12 b      

    { "cv ^"                       , 0                           },  // 13 g                                                
    { "fa ^"                       , 0                           },  // 14 e                                                   
    { "cx ^"                       , 0                           },  // 15 c      
    { "cz ^"                       , 0                           },  // 16 a      

// --- 4 stationary light ----
    { "."                          , 0                           },  // 01 l*     does not exist on 14 button
    { "."                          , 0                           },  // 02 p      
    { "."                          , 0                           },  // 03 n      
    { "."                          , 0                           },  // 04 j                                               

    { "."                          , 0                           },  // 05 k*     does not exist on 14 button
    { "ce ^"                       , 0                           },  // 06 o      
    { "cw ^"                       , 0                           },  // 07 m      
    { "."                          , 0                           },  // 08 i                                               

    { "."                          , 0                           },  // 09 h                                                
    { "."                          , 0                           },  // 10 f                                                                      
    { "."                          , 0                           },  // 11 d      
    { "."                          , 0                           },  // 12 b      

    { "."                          , 0                           },  // 13 g                                                
    { "cy ^"                       , 0                           },  // 14 e                                                   
    { "cu ^"                       , 0                           },  // 15 c      
    { "cq ^"                       , 0                           },  // 16 a      

// --- 5 stationary 001 ------
    { "."                          , 0                           },  // 01 l*     does not exist on 14 button
    { "."                          , 0                           },  // 02 p      
    { "."                          , 0                           },  // 03 n      
    { "."                          , 0                           },  // 04 j                                               

    { "."                          , 0                           },  // 05 k*     does not exist on 14 button
    { "."                          , 0                           },  // 06 o      
    { "."                          , 0                           },  // 07 m      
    { "."                          , 0                           },  // 08 i                                               

    { "."                          , 0                           },  // 09 h                                                
    { "."                          , 0                           },  // 10 f                                                                      
    { "."                          , 0                           },  // 11 d      
    { "gs"                         , 0                           },  // 12 b      

    { "."                          , 0                           },  // 13 g                                                
    { "."                          , 0                           },  // 14 e                                                   
    { "ge"                         , 0                           },  // 15 c      
    { "ga"                         , 0                           },  // 16 a      

    // not pre-acked as mobile server can't act upon it anyway
// --- 6 mobile player -------
    { "."                          , 0                           },  // 01 l*     does not exist on 14 button
    { "."                          , 0                           },  // 02 p      
    { "zj"                         , 0                           },  // 03 n      
    { "zr"                         , 0                           },  // 04 j                                               

    { "."                          , 0                           },  // 05 k*     does not exist on 14 button
    { "zv"                         , 0                           },  // 06 o      
    { "zl"                         , 0                           },  // 07 m      
    { "zt"                         , 0                           },  // 08 i                                               

    { "zx"                         , 0                           },  // 09 h                                                
    { "zn"                         , 0                           },  // 10 f                                                                      
    { "zm"                         , 0                           },  // 11 d      
    { "zk"                         , 0                           },  // 12 b      

    { "zc"                         , 0                           },  // 13 g                                                
    { "zb"                         , 0                           },  // 14 e                                                   
    { "zp"                         , 0                           },  // 15 c      
    { "zh"                         , 0                           },  // 16 a      

    // not pre-acked as mobile server can't act upon it anyway
// --- 7 mobile access -------
    { "."                          , 0                           },  // 01 l*     does not exist on 14 button
    { "."                          , 0                           },  // 02 p      
    { "gc"                         , 0                           },  // 03 n      
    { "."                          , 0                           },  // 04 j                                               

    { "."                          , 0                           },  // 05 k*     does not exist on 14 button
    { "."                          , 0                           },  // 06 o      
    { "gz"                         , 0                           },  // 07 m      
    { "gn"                         , 0                           },  // 08 i                                               

    { "."                          , 0                           },  // 09 h                                                
    { "."                          , 0                           },  // 10 f                                                                      
    { "gd"                         , 0                           },  // 11 d      
    { "zz"                         , 0                           },  // 12 b      

    { "gb"                         , 0                           },  // 13 g                                                
    { "gv"                         , 0                           },  // 14 e                                                   
    { "gx"                         , 0                           },  // 15 c      
    { "bb"                         , 0                           },  // 16 a      

    // not pre-acked as mobile server can't act upon it anyway
// --- 8 mobile door ---------
    { "."                          , 0                           },  // 01 l*     does not exist on 14 button
    { "."                          , 0                           },  // 02 p      KEY_CODE_META
    { "kx"                         , 0                           },  // 03 n      e multi_s_op_rm                          
    { "."                          , 0                           },  // 04 j                                               

    { "."                          , 0                           },  // 05 k*     does not exist on 14 button
    { "."                          , 0                           },  // 06 o      KEY_CODE_MOBL_DOOR_MENU
    { "kv"                         , 0                           },  // 07 m      KEY_CODE_MOBL_PLAY_MENU / e I_D_CLU
    { "."                          , 0                           },  // 08 i                                               

    { "."                          , 0                           },  // 09 h                                                
    { "."                          , 0                           },  // 10 f                                                                      
    { "kn"                         , 0                           },  // 11 d      e multi_s_op_4
    { "km"                         , "kc"                   },  // 12 b      e I_D_OPL

    { "."                          , 0                           },  // 13 g                                                
    { "."                          , 0                           },  // 14 e                                                   
    { "kh"                         , 0                           },  // 15 c      e I_D_OPU
    { "kp"                         , "kc"                   },  // 16 a      e I_D_OPUL

};

raw2menu_str  
key_raw2menu[] = {

    { },  
    // menu access methods used for server communication
    {_AT_ROTA2G_to_RPID,  0 },  // 01 l (META)          does not exist on 14 button
    {_AT_ROTA2G_to_RPID,  0 },  // 02 p (META)          
    { AT_TETHER_to_KARRp, 7 },  // 03 n (META)          
    {_AT_ROTA2G_to_RPID,  0 },  // 04 j (META)          

    {_AT_ROTA2G_to_RPID,  0 },  // 05 k (META)          does not exist on 14 button
    { AT_TETHER_to_KARRd, 8 },  // 06 o (META)          KEY_CODE_MOBL_DOOR_MENU
    { AT_TETHER_to_KARRp, 6 },  // 07 m (META)          KEY_CODE_MOBL_PLAY_MENU
    { AT_ESPNOW_to_PROX,  0 },  // 08 i (META)          

    {_AT_ROTA2G_to_RPID,  0 },  // 09 h (META)          
    {_AT_ROTA2G_to_RPID,  0 },  // 10 f (META)          
    {_AT_ROTA2G_to_RPID,  0 },  // 11 d (META)          
    { AT_ESPNOW_to_PROX,  2 },  // 12 b (META)          // 1: trigger direct p-command mechanism

    { AT_ESPNOW_to_PROX,  5 },  // 13 g (META)          
    { AT_ESPNOW_to_PROX,  4 },  // 14 e (META)          
    { AT_ESPNOW_to_PROX,  3 },  // 15 c (META)          
    { AT_ESPNOW_to_PROX,  1 },  // 16 a (META)          
};

#endif  // ENTITY_15_CLASS

// these are handled the same
#if ESP32_(10) || \
    ESP32_(12) || \
    ENTITY_15_CLASS || \
    ENTITY_45_CLASS || \
    ESP32_(53)
                                // --- 16b button remote control ---
                                // --- 14b button remote control ---
                                // --- 14c/d/e/f/g/h/i button remote control ---
                                // --- 16c button remote control ---
_u8 key_sns[] = {   // cols
    KEY_SNS_0,
    KEY_SNS_1,
    KEY_SNS_2,
    KEY_SNS_3,
};

_u8 key_hot[] = {   // rows
    KEY_HOT_0,
    KEY_HOT_1,
    KEY_HOT_2,
    KEY_HOT_3,
};

// return input pin portion of gpio only (lower four bits, all located within first 0-31)
// assemble consecutively to allow mapping of wiring to readable codes

#if defined(CONFIG_IDF_TARGET_ESP32C5)
#define GPIO_in GPIO.in.val
#define GPIO_out_low GPIO.out_w1tc.val
#define GPIO_out64_low GPIO.out1_w1tc.val
#define GPIO_out_high GPIO.out_w1ts.val
#define GPIO_out64_high GPIO.out1_w1ts.val
#define GPIO_oe_low GPIO.enable_w1tc.val
#define GPIO_oe64_low GPIO.enable1_w1tc.val
#define GPIO_oe_high GPIO.enable_w1ts.val
#define GPIO_oe64_high GPIO.enable1_w1ts.val
#else
#define GPIO_in GPIO.in
#define GPIO_out_low GPIO.out_w1tc
#define GPIO_out64_low GPIO.out1_w1tc.val
#define GPIO_out_high GPIO.out_w1ts
#define GPIO_out64_high GPIO.out1_w1ts.val
#define GPIO_oe_low GPIO.enable_w1tc
#define GPIO_oe64_low GPIO.enable1_w1tc.val
#define GPIO_oe_high GPIO.enable_w1ts
#define GPIO_oe64_high GPIO.enable1_w1ts.val
#endif

#if ESP32_(10)  // KEY_HOT_0 > 31 && KEY_HOT_1 > 31 / exception for ESP32_(10) // --- 16b button remote control ---
#define ASSEMBLE_SCAN_PINS() \
    ((GPIO.in1.val & 1 << KEY_HOT_0 - 32) >> KEY_HOT_0 - 32 << 3 \
   | (GPIO.in1.val & 1 << KEY_HOT_1 - 32) >> KEY_HOT_1 - 32 << 2 \
   | (GPIO_in & 1 << KEY_HOT_2 ) >> KEY_HOT_2 << 0 \
   | (GPIO_in & 1 << KEY_HOT_3 ) >> KEY_HOT_3 << 1)
#else           // KEY_HOT_0 <= 31 && KEY_HOT_1 <= 31 / all others
#define ASSEMBLE_SCAN_PINS() \
    ((GPIO_in & 1 << KEY_HOT_0 ) >> KEY_HOT_0 << 3 \
   | (GPIO_in & 1 << KEY_HOT_1 ) >> KEY_HOT_1 << 2 \
   | (GPIO_in & 1 << KEY_HOT_2 ) >> KEY_HOT_2 << 0 \
   | (GPIO_in & 1 << KEY_HOT_3 ) >> KEY_HOT_3 << 1)
#endif

#if DEBUG > 10
// return all pin states (8 bits, specific to this control)
_u32
dump_pins(_u8 dump)
{
    _u32 ret;

    ret = 
#if KEY_SNS_0 > 31
            (GPIO.in1.val & 1 << KEY_SNS_0 - 32) >> KEY_SNS_0 - 32 << 7
#else
            (GPIO_in      & 1 << KEY_SNS_0     ) >> KEY_SNS_0      << 7
#endif
          | (GPIO_in      & 1 << KEY_SNS_1     ) >> KEY_SNS_1      << 6
          | (GPIO_in      & 1 << KEY_SNS_2     ) >> KEY_SNS_2      << 5
          | (GPIO_in      & 1 << KEY_SNS_3     ) >> KEY_SNS_3      << 4

#if KEY_HOT_0 > 31
          | (GPIO.in1.val & 1 << KEY_HOT_0 - 32) >> KEY_HOT_0 - 32 << 3
#else
          | (GPIO_in      & 1 << KEY_HOT_0     ) >> KEY_HOT_0      << 3
#endif
#if KEY_HOT_1 > 31
          | (GPIO.in1.val & 1 << KEY_HOT_1 - 32) >> KEY_HOT_1 - 32 << 2
#else
          | (GPIO_in      & 1 << KEY_HOT_1     ) >> KEY_HOT_1      << 2
#endif
          | (GPIO_in      & 1 << KEY_HOT_2     ) >> KEY_HOT_2      << 0
          | (GPIO_in      & 1 << KEY_HOT_3     ) >> KEY_HOT_3      << 1;
    if (dump) printf("<" BIN_FORMAT8 ">  ", BIN_VALUE8(ret));
    return ret;
}
#endif

#if ENTITY_45_CLASS

// this should save 4 pull ups on the KEY_HOT pins
void
hot_keys_output_pull_high_in_deep_sleep(_u32 enable)
{
TP05
    PR05(" -> %sdo\n", enable ? "" : "un");
    if (enable) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = KEY_HOT_MASK,
            .pull_down_en = 0,      // disable to not cause leak currents
            .pull_up_en = 0,        // disable to not cause leak currents
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        GPIO.out_w1ts = KEY_HOT_MASK;

        ESP_ERROR_CHECK(gpio_hold_en(KEY_HOT_0));
        ESP_ERROR_CHECK(gpio_hold_en(KEY_HOT_1));
        ESP_ERROR_CHECK(gpio_hold_en(KEY_HOT_2));
        ESP_ERROR_CHECK(gpio_hold_en(KEY_HOT_3));
        gpio_deep_sleep_hold_en();  // no err check supported
    } else {
        // pins do not work properly if this stays enabled for non deep sleep
        ESP_ERROR_CHECK(gpio_hold_dis(KEY_HOT_0));
        ESP_ERROR_CHECK(gpio_hold_dis(KEY_HOT_1));
        ESP_ERROR_CHECK(gpio_hold_dis(KEY_HOT_2));
        ESP_ERROR_CHECK(gpio_hold_dis(KEY_HOT_3));
        gpio_deep_sleep_hold_dis();  // no err check supported/ ALSO WORKS WITHOUT!
    }
}

// this should save 4 pull downs on the KEY_SNS pins
void
sns_keys_input_pull_down_in_deep_sleep(_u32 enable)
{
TP05
    PR05(" -> %sdo\n", enable ? "" : "un");
    if (enable) {
        rtc_gpio_pullup_dis(KEY_SNS_0);
        rtc_gpio_pulldown_en(KEY_SNS_0);
        rtc_gpio_pullup_dis(KEY_SNS_1);
        rtc_gpio_pulldown_en(KEY_SNS_1);
        rtc_gpio_pullup_dis(KEY_SNS_2);
        rtc_gpio_pulldown_en(KEY_SNS_2);
        rtc_gpio_pullup_dis(KEY_SNS_3);
        rtc_gpio_pulldown_en(KEY_SNS_3);
    } else {
        rtc_gpio_pullup_dis(KEY_SNS_0);
        rtc_gpio_pulldown_dis(KEY_SNS_0);
        rtc_gpio_pullup_dis(KEY_SNS_1);
        rtc_gpio_pulldown_dis(KEY_SNS_1);
        rtc_gpio_pullup_dis(KEY_SNS_2);
        rtc_gpio_pulldown_dis(KEY_SNS_2);
        rtc_gpio_pullup_dis(KEY_SNS_3);
        rtc_gpio_pulldown_dis(KEY_SNS_3);
    }
}
#endif  // if ENTITY_45_CLASS

#if defined(LIGHT_SLEEP_TIMEOUT)
/*
 * prepare keys for light sleep
 *
 *  HOT keys turn to ouput feeding key press signal to SNS keys (input) for wakeup
 *  this is due to the ext1 contraint: "wakeup on any high"
 */
_i32
prepare_keys_for_light_sleep(_u32 enable)
{
TP05
    PR05(" -> %sdo\n", enable ? "" : "un");
    if (enable) {
        // taken from esp-idf.v5.5.4/examples/peripherals/gpio/generic_gpio/main/gpio_example_main.c
        gpio_config_t io_conf_inputs = {       // pulls and intrs are auto. set off
            .mode = GPIO_MODE_INPUT,
            .pin_bit_mask = KEY_SNS_MASK,
            .pull_down_en = 1,                  // pull down, wait for high
            .pull_up_en = 0,
            .intr_type = GPIO_INTR_ANYEDGE,
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf_inputs));

        gpio_config_t io_conf_outputs = {       // pulls and intrs are auto. set off
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = KEY_HOT_MASK,
            .pull_down_en = 0,                  // pull down, wait for high
            .pull_up_en = 0,
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf_outputs));

        GPIO_out_high = KEY_HOT_MASK;               // set active high
    }
    return 0;
}
#endif  //  defined(LIGHT_SLEEP_TIMEOUT)

/*
 * prepare keys for scanning key codes
 *
 *  HOT keys turn to input defaulting to high pulled low by sequential SNS key scan (output) on pressed cross connects
 *  this is due to the resistor network (voltage didider) providing a high signal if both HOT and SNS are inputs and subected to their internal/external pull resistors only 
 *
 *  this effectively reverses the operation direction of HOT and SNS keys
 *
 */
void
init_keys()
{
TP05
    gpio_config_t io_conf = {       // pulls and intrs are auto. set off
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = KEY_HOT_MASK,

#if !ESP32_(53) 
        // pull_up/ pull_down for all except ESP32_(53):
        // on ESP32_(46) there are no external pull up Rs at all -> enable this
        // on other ESP32_() strengthen the external pull up Rs (for multi key press)
        // as default level for SNS/input HOT/input must result in HIGH level
        // as LOW level is introduced by active SNS/output
        // ESP32_(53) delivers already 7.5k / 47k ratio -> no additional pullup required
        .pull_down_en = 0,
        .pull_up_en = 1,
#endif

    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

_u32
scan_keys()
{
TP05
    _u32 key;
    _u32 cnt;

    key = 0;

    /*
     * configure the SNS pins as inputs ONCE instead of once per scan round
     *
     * gpio_config() runs rtc_gpio_deinit() for every RTC capable pin of the mask. on the C5
     * that gates the clock of the separate LP IO domain (SOC_LP_PERIPHERALS_SUPPORTED) and
     * costs ~10ms per pin. with all four SNS pins RTC capable (GPIO 3/0/1/6) the old
     * "config 4 inputs + config 1 output" per round summed up to ~250ms for a single scan.
     * the per round direction flipping is done by register below instead.
     */
    gpio_config_t io_conf_inputs = {       // pulls and intrs are auto. set off
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = KEY_SNS_MASK,
        .pull_down_en = 0,              // only outputs are actively driven low (next lines)
/*
 * not required for traditional (AKA non-super-mini) u-remotes as 
 *  1. these have their pulls built in
 *  2. pull survives the deep sleep causing an instant wakeup on traditional u-remotes
 *     if not undone right before deep sleep (what is not done currently)
 * for super-mini u-remotes pull_up is required as
 *  1. these do not have external pulls
 *  2. pins are prepared explicitly per sns_keys_input_pull_down_in_deep_sleep(1) for deep sleep
 *       rtc_gpio_pullup_dis(KEY_SNS_0);
 *       rtc_gpio_pulldown_en(KEY_SNS_0);
 *     pulling the sense actively down -> preventing instant wakeup
 */
#if ENTITY_45_CLASS
        .pull_up_en = 1,                // inputs basically should stay up
#endif
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf_inputs));

    for (cnt = 0; cnt < 4; ++cnt) {
        /*
         * turn ONE SNS pin into a low driving output and back to input by register only.
         * the latch is driven low BEFORE the output enable to avoid a short high glitch.
         * the replaced gpio_config() pair incidentally provided ~20ms of settling time
         * between driving the SNS line and reading the HOT lines -> settle explicitly now.
         */
        if (key_sns[cnt & 3] > 31) {
            GPIO_out64_low         = 1 << key_sns[cnt & 3] - 32;
            GPIO_oe64_high         = 1 << key_sns[cnt & 3] - 32;
        } else {
            GPIO_out_low           = 1 << key_sns[cnt & 3];
            GPIO_oe_high           = 1 << key_sns[cnt & 3];
        }

        esp_rom_delay_us(KEY_SCAN_SETTLE_US);

        key = key << 4 | ASSEMBLE_SCAN_PINS() ^ 0b00001111; // depressed keys are active low (located in lower 4 bits)

        if (key_sns[cnt & 3] > 31) {
            GPIO_oe64_low          = 1 << key_sns[cnt & 3] - 32;
        } else {
            GPIO_oe_low            = 1 << key_sns[cnt & 3];
        }
    }
    PR11("phys butt: " BIN_FORMAT16 "\n", BIN_VALUE16(key));
    // in case multiple simultaneous keys need to be supported move ffs() to calling routines
    key = ffs(key);
#if ESP32_(10)
    key = tr_keys[key];     // make key codes all appear the same for all devices
#endif

#ifdef DEBUG_TIMER_WAKEUP
    SIMULATE_KEY_PRESSES_WITH(KEY_BOARD_SIMU_KEY);
#endif

    return key;
}
#endif      // #if ESP32_(10)...
// -------------------------------------------------------------------------------------

_i32
prepare_keys_for_deep_sleep(_u32 enable)
{
TP05
    PR05(" -> %sdo\n", enable ? "" : "un");
    if (enable) {
        /*
         * prepare keys for deep sleep
         */
#if defined(DEBUG_TIMER_WAKEUP)  // --- profiling: wake on a timer, not on the keys ---
        PR01("DEBUG_TIMER_WAKEUP: %dms deep sleep\n", DEBUG_TIMER_WAKEUP);
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(DEBUG_TIMER_WAKEUP * 1000ULL));
#elif ESP32_(9)                 // --- 1 button remote control ---
        ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(KEY_SNS, 0));     // 1 = High, 0 = Low
#else                           // --- all others ---
        ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(KEY_SNS_MASK, ESP_EXT1_WAKEUP_ANY_HIGH));
#endif
#if defined(VBAT_PATROL_INTERVAL) && !defined(DEBUG_TIMER_WAKEUP)
        /*
         * a second wakeup source NEXT TO the keys, not instead of them:
         * esp_sleep_enable_timer_wakeup() only ORs RTC_TIMER_TRIG_EN into s_config.wakeup_triggers,
         * so both stay armed and esp_sleep_get_wakeup_cause() tells them apart on the way back up
         *
         * the ULL is load bearing: 5 * 3600 * 1000000 overflows a 32 bit int and would silently
         * leave you with an 89 minute period
         *
         * ATTENTION: this re-arms on EVERY deep sleep entry, so a key press postpones the next
         * patrol by a full interval. that is deliberate - a key press is a battery check itself
         * (see the vbat block at the end of process_input()), so what the patrol has to cover is
         * exactly the idle stretches
         */
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(VBAT_PATROL_INTERVAL * 1000000ULL));
#endif
        /*
         * as no external pulls exist on these devices (s3_s_mini) -> try to use the internals
         */
#if ENTITY_45_CLASS
        hot_keys_output_pull_high_in_deep_sleep(1);
        sns_keys_input_pull_down_in_deep_sleep(1);
#endif
    } else {
#if ENTITY_45_CLASS
        hot_keys_output_pull_high_in_deep_sleep(0);
        sns_keys_input_pull_down_in_deep_sleep(0);
#endif
    }
    return 0;
}

_u32
wait_for_key_release()
{
TP05
    _u32 key;
    _u32 cnt = 0;

    while (key = scan_keys()) {
        PR06("key: 0x%02x\n", key);
        vTaskDelay(pdMS_TO_TICKS(100));
        ++cnt;
    }
    PR02("key RELEASED after [%d]\n", cnt);
    return cnt;
}
/* ---^^^--- KEY section ---^^^--- */

#if defined(VBAT_ADC1_SENSE_PIN)

#if ENTITY_45_CLASS
/*
 * account for LiPo batteries
 */
#define VBAT_TRIGGER_LOW_LIMIT      2000    // toss unplausible vals
#define VBAT_TRIGGER_HIGH_LIMIT     5000
#define VBAT_TRIGGER_INITIAL        4900    // force an initial trigger if below that
#else
#define VBAT_TRIGGER_LOW_LIMIT      2000    // toss unplausible vals
#define VBAT_TRIGGER_HIGH_LIMIT     3500
#define VBAT_TRIGGER_INITIAL        2910    // force an initial trigger if below that
#endif

    RTC_DATA_ATTR _i32 vbat_trigger = VBAT_TRIGGER_INITIAL;
    RTC_DATA_ATTR _i32 vbat         = VBAT_TRIGGER_INITIAL;

/*
 * sample the cell into vbat
 *
 * factored out of process_input() when the accu patrol arrived: the regular path samples with
 * WiFi up (AKA under load), the patrol samples with the radio still down, and both have to go
 * through the very same divider/bias arithmetic or the two readings cannot be compared at all
 */
_i32
vbat_sample()
{
TP05
#if defined(CONFIG_IDF_TARGET_ESP32C5)

    _i32 adc_raw[2][10];
    _i32 voltage[2][10];

    // must consider bias for this and can't use adc_oneshot_get_calibrated_result() convenience fct
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_unit, VBAT_ADC1_SENSE_PIN, &adc_raw[0][0]));
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali, adc_raw[0][0], &voltage[0][0]));
    vbat = voltage[0][0];
#else
    ESP_ERROR_CHECK(adc_oneshot_get_calibrated_result(adc1_unit, adc1_cali, VBAT_ADC1_SENSE_PIN, &vbat));
#endif
    // account for 1:1 resistor divider
    vbat <<= 1;

// vbat simulation/ programmed 2023/05/27 in Spain/ Andalusia/ Mijas 
#ifdef VBAT_DRAIN_SIMU
    vbat = min(3100, vbat_trigger - 1);
#endif
    return vbat;
}

/*
 * report vbat and pick up the new trigger limit the server hands back
 *
 * the server ratcheting vbat_trigger DOWN is what keeps the alarm from repeating forever.
 * mysend() always fills statmsg - with _err[stat] on failure - and atoi() of that lands far below
 * VBAT_TRIGGER_LOW_LIMIT, so a failed report simply leaves the trigger alone and we retry next time
 */
_i32
vbat_report(access_target_str *at_ptr, _u32 adapt_trigger)
{
TP05
    _i8 cmd[64];
    _i8p statmsg;
    _i32 ret;

    /*
     * TO FIX!
     * for continued support of the existent API on the smarthone 
     * we must adapt the VBat string sent accordingly
     */
    if (!strcmp(at_ptr->host, KARR_TARGET_HOST)) {
        snprintf(cmd, _SZ(cmd), "@VBat"                      "=" "%d"     , vbat);
    } else {
        snprintf(cmd, _SZ(cmd), "@VBat" "_" CDEF2STR(ENTITY) "=" "%d" " ^", vbat);
    }
    if (ret = mysend(cmd, at_ptr->host, at_ptr->port, &statmsg)) {
        PR00("could not send [%s] to [[%s]:%d]\n", cmd, at_ptr->host, at_ptr->port);
    }
    /*
     * only the caller knows whether statmsg came end to end from the target application or from
     * the first gateway that felt like answering - see VBAT_PATROL_CRITICAL. a pre-acked cmd
     * carries a uniform status whose STAT_MISC has nothing to do with this battery, and letting
     * that through min() would silently drag vbat_trigger down for good
     */
    if (!adapt_trigger) {
        return ret;
    }
    // set new trigger limit but ignore unplausible vals
    if (atoi(statmsg) > VBAT_TRIGGER_LOW_LIMIT \
     && atoi(statmsg) < VBAT_TRIGGER_HIGH_LIMIT) {
        vbat_trigger = min(atoi(statmsg), vbat_trigger);
        PR02("VTrigger update: %dmV\n", vbat_trigger);
    }
    return ret;
}
#endif

#if defined(SUPPORT_MENU_SWITCHING)

//
// expects all key meta information stripped
//
_u32
switch_dynamic_access_target(_u32 key)
{
TP05
    if (key_raw2menu[key].access_target_index == AT_NOT_IN_USE) {
        PR00("menu key pressed: not used, ignoring\n");
        key = 0;
    } else {
        PR02("menu key pressed: 0x%02x access_target_index: 0x%02x\n", key, key_raw2menu[key].access_target_index);
        // save planned access target temporarily -> will be taken over permanently if acknowledged 
        _dynamic_access_target = key_raw2menu[key].access_target_index;
    }
    return key;
}

#endif  // defined(SUPPORT_MENU_SWITCHING)

/*
special version of beep signs typically called from loops
*/
#define _PULSEWIDTH 30000
#define _PULSEPITCH 6000
#define _PULSEVOL 4095
#define _PULSEPURG 100
#define _PULSEINTERVAL 500

_i32 _pulsevol;
_i32 _pulsewidth;
__i32 pulsevol_NVS;
__i32 pulsewidth_NVS;
RTC_DATA_ATTR _i32 pulsevol_RTC;
RTC_DATA_ATTR _i32 pulsewidth_RTC;

void
interval_beep()
{
TP05
    static _u32 x;

    if (tstamp() - x > _PULSEINTERVAL) {
        beep_enque(_pulsewidth, _PULSEPITCH, _pulsevol);
        beep_enque(_PULSEPURG, 0, 0);
        x = tstamp();
    }
}

_i32
deinit_WiFi()
{
TP05
    ESP_ERROR_CHECK(ur_disconnect());
    return 0;
}

_i32
deinit_ESPNOW()
{
TP05
    //ESPNOW_DEL_PEER(espnow_gateway_mac);
    ESPNOW_DEINIT();
    //ESP_ERROR_CHECK(esp_wifi_UNset_max_tx_power(ATTENTION_REDUCED_WIFI_POWER));
    ESPNOW_WIFI_UNSETUP();
    return 0;
}

_i32
init_WiFi(access_target_str *at_ptr)
{
TP05
    _i32 ret = 0;

    /*
     * we always start out with moderate aggressive params as we still probe the connection
     * if the probe has proven to be successful we upgrade later (affects light sleep only)
     * choose the maximum power save
     */
    // WIFI_PS_MAX_MODEM: 37mA in light sleep
    // WIFI_PS_MIN_MODEM: 37mA in light sleep, often peaks to 59mA
    // WIFI_PS_NONE:     104mA in light sleep 




// must distinct dep sleep WiFi and lightsleep WiFi?!?!?
// must distinct dep sleep WiFi and lightsleep WiFi?!?!?
// must distinct dep sleep WiFi and lightsleep WiFi?!?!?
// must distinct dep sleep WiFi and lightsleep WiFi?!?!?
//  DEEP ->  WIFI_PS_NONE
//  LIGHT -> WIFI_PS_MAX_MODEM




    if (ur_connect(at_ptr->accp, !WIFI_CONN_WAIT, WIFI_CONN_MODERATE_FAIL, WIFI_PS_MAX_MODEM)) {
        PR00("could not connect to %s\n", GET_SSID(at_ptr->accp));
        ret = 1;
    }
    return ret;
}

_i32
init_ESPNOW()
{
TP05
    ESPNOW_WIFI_SETUP();
    ESPNOW_INIT();
    return 0;
}

_i32
wireless(access_target_str *at_ptr)
{
TP05
    _i32 ret = 0;

#define WAS_NONE   0
#define WAS_WIFI   1
#define WAS_ESPNOW 2

static _u32 last_tech;

PR10("last_tech pre: %d\n", last_tech);
    if (!at_ptr) {      // AKA stop it
        if (last_tech == WAS_NONE) {
            // all done
        } else {
            if (last_tech == WAS_ESPNOW) {
                //deinit_ESPNOW();
                //simple version of that:
/*
 * SKIP_WIRELESS_OFF_BEFORE_DEEP_SLEEP is decided HERE, not at the call sites:
 *  - last_tech says what is ACTUALLY up, which beats isESPNOW(at_ptr->host) at a call site (that
 *    only says what we intended this cycle), and resort_to_deep_sleep() has no at_ptr at all -
 *    which is exactly how a call site got missed when this lived at the call sites (2026_08_25)
 *  - only the ESPNOW stop is free to skip. the WiFi branch below is deinit_WiFi() ->
 *    ur_disconnect() and is LOAD BEARING: a dangling association costs the next wakeup ~1.6s
 *  - and only before DEEP sleep: before light sleep the driver must really stop, else the
 *    last_tech = WAS_NONE below is a lie and the next wireless(at_ptr) re-inits a started driver
 */
#if defined(SKIP_WIRELESS_OFF_BEFORE_DEEP_SLEEP)
                if (!WIRELESS_OFF_IS_BEFORE_DEEP_SLEEP())
#endif
                ESP_ERROR_CHECK(esp_wifi_stop());
            } else {    // if (last_tech == WAS_WIFI) {
                deinit_WiFi();
                //simple version of that:

//would that woek???
//ESP_ERROR_CHECK(esp_wifi_stop());

            }
last_tech = WAS_NONE;
        }
    } else if (isESPNOW(at_ptr->host)) {
        if (last_tech == WAS_ESPNOW) {
            // all done
        } else {
            if (last_tech == WAS_WIFI) {
                deinit_WiFi();
last_tech = WAS_NONE;
            }
            init_ESPNOW();
last_tech = WAS_ESPNOW;
        }
    } else {
        if (last_tech == WAS_WIFI) {
            // all done
        } else {
            if (last_tech == WAS_ESPNOW) {
                deinit_ESPNOW();
last_tech = WAS_NONE;
            }
            if (init_WiFi(at_ptr)) {
                ret = 1;
                goto out;
            }
last_tech = WAS_WIFI;
        }
    }
out:
PR10("last_tech post: %d\n", last_tech);
    return ret;
}

#if defined(VBAT_PATROL_INTERVAL)

#if !defined(VBAT_PATROL_CHIRP)
// set to zero as mysend emits a signal anyway on err/ok
#define VBAT_PATROL_CHIRP 0     // local low accu alarm, beeps. set to 0 to keep the patrol mute
#endif

/*
 * periodic accu patrol
 *
 * we get here on a deep sleep TIMER wakeup, AKA nobody pressed anything and there is nothing to
 * send. so sample first with the radio still down - that is a few hundred us - and only pay for a
 * transmission once the cell has actually dropped into the alarm band. a silent patrol is noise
 * against the idle current; one that associates unconditionally is not.
 *
 * the gate is VBAT_PATROL_CRITICAL, a constant measured against the UNLOADED sample - see the
 * define for why a ratcheted vbat_trigger cannot work on this path. what gets REPORTED is
 * re-sampled with the radio on, so the server sees the same under-load number the regular path
 * reports. nothing is adapted from the answer: below the threshold we just keep reporting every
 * interval until somebody charges the device, and the alarm ends when the cell comes back up.
 *
 * the target is whatever menu the remote was last left in. at home that is AT_ESPNOW_to_PROX,
 * which costs a fraction of an association and whose gateway is always up. only out on the road
 * does this become KARR_TARGET_HOST - and that host exists only while the phone is tethering,
 * which is exactly when the remote is NOT in deep sleep. pin it to
 * &access_target[AT_TETHER_to_KARRp] here if you want the mobile target unconditionally, but
 * expect every patrol at home to burn an association that cannot succeed
 */
_i32
vbat_patrol()
{
TP05
    access_target_str *at_ptr = &access_target[dynamic_access_target];

    vbat_sample();
    PR02("patrol VBat: %dmV, VCritical: %dmV\n", vbat, VBAT_PATROL_CRITICAL);
    if (vbat >= VBAT_PATROL_CRITICAL) {
        return 0;   // healthy -> straight back to deep sleep, the radio never came up at all
    }
    PR01("patrol: undervoltage -> reporting to [0x%02x:[%s]:%d]\n", at_ptr->accp, at_ptr->host, at_ptr->port);
    if (VBAT_PATROL_CHIRP) {
        beep(BEEP_ERR, VBAT_PATROL_CHIRP);  // the only alarm that works when nothing is reachable
    }
    if (!wireless(at_ptr)) {
        vbat_sample();          // re-sample under load, same number the regular path reports
        vbat_report(at_ptr, 0); // 0: pre-acked over ESPNOW -> statmsg is not from the final target
    }
    wireless(0);        // what is safe to skip is decided inside wireless(), not here
    return 0;
}
#endif  // if defined(VBAT_PATROL_INTERVAL)

_i32
process_input(_u32 key)
{
TP05
    access_target_str *at_ptr;

    //led_act(1);
    //rgb_red(1);
    //rgb_green(1);
    //rgb_blue(1);

    // indicate keypress without delay to reflect hardware wear
    beep_enque(BEEP_SPIKE_PULSE_WIDTH, BEEP_SPIKE, BEEP_VOLUME_SPIKE);
    beep_enque(BEEP_PURGE_PULSE, 0, 0);
/*
 * the zero point the ear measures from. the chirp is BEEP_SPIKE_PULSE_WIDTH == 30ms of audio and
 * dispatch_beeps blocks on an empty queue afterwards, so the status tone sounds continuous with it
 * only if mysend()'s beep(BEEP_OK, 1) is enqueued within those 30ms. PR01 because these entities
 * build with DEBUG 1 - anything higher compiles to nothing on the very serial line we measure from
 */
PR01("chirp: %lu\n", tstamp());
    _dynamic_access_target = dynamic_access_target; // init the dynamic version with current, may be overwritten shortly

// -----------------------------------------------------
    //
    // ADAPT COMMENT - KEY_CODE_META not supported on performane critical stuff (as no wait for timeout is feasible)
    //
    // - wait for the specified timeout to expire to
    //   get the final situation which is one of:
    //      - an addtional (i.e. a second) key is depressed
    //      - no addtional key is depressed (i.e. no change)
    //      - no longer any key is depressed at all (i.e. all released)
    // - but do not wait at all if in AT_TETHER_to_KARRd since all keys then are forwarded as is (due to real time door opening func)
    //
    // remotes with up to 4 keys should not support long key press feature at all
    // do not support long key press feature to NOT f..... performance tests with vTaskDelay() for
    //                  ESP32_(2) own pre crafted generic development board
    // do not support long key press feature because of crashing in key_raw2cmd[] (because it's not fully populated) for
    //                  ESP32_(2) own pre crafted generic development board
    //                  ESP32_(9)
    //                  ESP32_(11)
    //
    // remotes with more than 4 keys basically support long key press feature BUT
    // do not evaluate long key press feature if currently in AT_TETHER_to_KARRd for:
    //      ESP32_(10) ultra_smartremote_16b
    //      ESP32_(12) ultra_smartremote_14b
    //      ESP32_(15) ultra_smartremote_14c
    //      ESP32_(13) ultra_smartremote_14e
    //      ESP32-(14) ultra_smartremote_14d
    //      ESP32-(21) ultra_smartremote_16c
    // due to the opener key support feature
    // as a consequence this effectively disallows MENU switching in AT_TETHER_to_KARRd mode (except per toggle)
    //

    /*
     * as the introducing spike beep now has been issued we may bail out if none is pressed
     */
    if (!key) { 
        goto out;
    }
#if defined(SUPPORT_MENU_SWITCHING) 
    if (key == KEY_CODE_META) {
        _u32 now;

        now = tstamp();
        while (tstamp() - now < MENU_SWITCH_INTRO_TIMEOUT) {
            // latch the key to minimize bouncing issues
            if (!(key = scan_keys())) { // key changes state -> debounce
//              vTaskDelay(pdMS_TO_TICKS(MENU_SWITCH_ALLOW_DEBOUNCE));      //  debouncing not required here
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(MENU_SWITCH_RESCAN_SPACING));
        }
        if (key) {
            /*
             *  got a timeout introducing a menu switch
             *  wait for key release without timeout
             */
            while (1) {
                interval_beep();
                if (!scan_keys()) { // key changes state -> debounce to not cause false key presses in successing stages
                    vTaskDelay(pdMS_TO_TICKS(MENU_SWITCH_ALLOW_DEBOUNCE));  // scan_keys() may jump back to 1 -> debounce  
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(MENU_SWITCH_RESCAN_SPACING));
            }
            // here we know: no key or non-KEY_CODE_META is pressed
            now = tstamp();
            while (tstamp() - now < MENU_SWITCH_TARGET_TIMEOUT) {
                interval_beep();
                if (key = scan_keys()) {
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(MENU_SWITCH_RESCAN_SPACING));
            }
            if (key) {
                key = switch_dynamic_access_target(key);
            } else {
                // timeout for menu button expired -> no further action, will get kicked out below
            }

        //
        // here we know KEY_CODE_META is pressed for a short time (indicating a menu switch FOR SPECIAL CASES ONLY)
        //
        // exception to support legacy mode (toggle on AT_TETHER_to_KARRp, AT_TETHER_to_KARRd)
        // AKA implicit switch between AT_TETHER_to_KARRp and AT_TETHER_to_KARRd on short press of MENU key
        //
        // HACK
        //  - to realize a menu toggle function between AT_TETHER_to_KARRp/d (specific to 16/14 button remote control)
        //  - since no long press menu switches are supportable in AT_TETHER_to_KARRd to leave the menu 
        //  - active only if you already are in some UR2_mobl_player or UR2_mobl_door menu
        //  - to also support defined start/shutdown of door proxy processes between menu changes
        //
        } else if (dynamic_access_target == AT_TETHER_to_KARRp) {  // use current dynamic_access_target (w/o _)
            // emulate KEY_CODE_MOBL_DOOR_MENU
            key = switch_dynamic_access_target(KEY_CODE_MOBL_DOOR_MENU);   // emulate M_PFX "o"
        } else if (dynamic_access_target == AT_TETHER_to_KARRd) {   // use current dynamic_access_target (w/o _)
            // emulate KEY_CODE_MOBL_PLAY_MENU
            key = switch_dynamic_access_target(KEY_CODE_MOBL_PLAY_MENU);   // emulate M_PFX "m"
        } else {
            /*
             * restore KEY_CODE_META since it may have been overwritten
             * 
             * gets here if menu switching timeout pressing the KEY_CODE_META
             * has not been reached and AT_TETHER_to_KARRp or AT_TETHER_to_KARRd not currently active
             * and thus temporarily has been set to zero
             *
             * ATTENTION: key now must be processed further as non-menu key
             */
            key = KEY_CODE_META;
            goto out1;
        }
        if (!key) { // a timed out menu switch could cause that
            goto out;
        }

        /*
         * a real menu key has been identified and now will be processed
         */
        PR02("processing menu key: [0x%02x/%s]\n", key, key_raw2cmd[key].cmd);
    } else
#endif
    {
#if defined(SUPPORT_MENU_SWITCHING) 
out1:
        key += dynamic_cmd_offset * CMD_KEY_OFFSET_MULTIPLIER;
#else
        // nothing to do
#endif
        PR02("processing cmd key: [0x%02x/%s]\n", key, key_raw2cmd[key].cmd);
    }
    at_ptr = &access_target[_dynamic_access_target];
    PR02("atptr: [0x%02x(%s):[%s]:%d]\n", 
            at_ptr->accp, 
            at_ptr->accp ? (*accpts[at_ptr->accp << 1] ? accpts[at_ptr->accp << 1] : "ESPNOW") : "nil access point", 
            at_ptr->host, 
            at_ptr->port);

    wireless(at_ptr);

// check for valid kays
if (*key_raw2cmd[key].cmd != '.') {

    /*
     * theory of operation:
     *
     * there are 2 categories of commands:
     * 1. regular p-command (routed via p-menu selection) e.g. [2C] (select UR2_stat_audio) + [xz] (audio off)
     *      <- with key_raw2menu[key].off==0
     * 2. direct p-command (routed explicitly per cmd) e.g. [oa ^host3.example.com:8899] (CMD_ldoor_open_signal_assert)
     *      <- with key_raw2menu[key].off==1
     *
     * even though p-menu selection cmd [2B] is NOT required for [2b] (np ^host3.example.com:8899) it is regularly sent to the server
     * reason: we want a regular accoustic feedback (cmd success/fail) when selecting direct p-command menues
     */

#if defined(VBAT_ADC1_SENSE_PIN)
    /*
     * UGLY HACK as of 2026_10_01: 
     * artificially trigger a voltage report and suppress the original cmd
     * TO BE FIXED!!!
     */
    if (!strcmp(key_raw2cmd[key].cmd, "gd")) {
        vbat_trigger = 10000;
    } else
#endif // if defined(VBAT_ADC1_SENSE_PIN)
    if (!mysend(key_raw2cmd[key].cmd, at_ptr->host, at_ptr->port, 0)) {
#if defined(SUPPORT_MENU_SWITCHING)
        if (IS_MENU_KEY(key)) {
            /*
             * mysend() announced the menu press (if any) successfully to the server -> so do the internal switch also
             * at this point we know a possible menu switch has been accepted by the server
             * so we sync ourselves permanently to the new server state / don't do that if mysend() failed
             *
             * write dynamic_cmd_offset and dynamic_access_target of last menu key to RTC_DATA_ATTR:
             */
            PR06("dynamic_cmd_offset: %d\n", dynamic_cmd_offset);
            dynamic_cmd_offset = key_raw2menu[key].off;
            if (dynamic_access_target != _dynamic_access_target) {
                PR06("UPDATING dynamic_access_target from %d -> %d\n", dynamic_access_target, _dynamic_access_target);
                dynamic_access_target = _dynamic_access_target;     // set due to menu key
            } else {
                PR06("keeping dynamic_access_target %d\n", dynamic_access_target);
            }
        }
#endif  // if defined(SUPPORT_MENU_SWITCHING)
    } else {
        PR00("could not send [%s] to [[%s]:%d]\n", key_raw2cmd[key].cmd, at_ptr->host, at_ptr->port);
    }
TPROF("main_send_done");

    /*
     * we must also send a closing cmd if the opening cmd requests that
     * no matter if the opening cmd failed
     * AKA send a key release cmd even if the press is reported to fail, for your safety
     */
    if (key_raw2cmd[key].cmdClosing) {
        wait_for_key_release();
        if (mysend(key_raw2cmd[key].cmdClosing, at_ptr->host, at_ptr->port, 0)) {
            PR00("could not send CLOSING [%s] to [[%s]:%d]\n", key_raw2cmd[key].cmdClosing, at_ptr->host, at_ptr->port);
        }
    }
TPROF("closing_done");
} else {
    beep(BEEP_ERR, 3);
    PR00("invalid cmd\n");
}

#if defined(LIGHT_SLEEP_TIMEOUT)
    /*
     * my_sleep_mode_is_deep not only possibly changes if IS_MENU_KEY() but also after
     * expiry of a light sleep timeout. so in case of any key press update 
     * my_sleep_mode_is_deep according to current dynamic_access_target setting
     * after a real menu switch anyway
     */
    if (my_sleep_mode_is_deep != MY_SLEEP_MODE_IS_DEEP()) {
        PR06("UPDATING my_sleep_mode_is_deep from %d -> %d\n", my_sleep_mode_is_deep, MY_SLEEP_MODE_IS_DEEP());
        my_sleep_mode_is_deep = MY_SLEEP_MODE_IS_DEEP();    // set due to menu key

        /*
         * we always start out with values suitable for battery operation (AKA failing fast) -> upgrade it now if feasible
         * otherwise failing WiFi would bail out too fast leaving the current connection unusable
         */
        if (!my_sleep_mode_is_deep) {
            PR06("upgrades WiFi to slow fail\n");
            ur_upgrade_max_retries(WIFI_CONN_SLOW_FAIL);
        }
    } else {
        PR06("keeping my_sleep_mode_is_deep %d\n", my_sleep_mode_is_deep);
    }
#endif

#if defined(VBAT_ADC1_SENSE_PIN)

/*
 * hack to restrict this to mobile devices ATM
 */
if (!strcmp(at_ptr->host, KARR_TARGET_HOST)) {

    /*
     * measure battery voltage under load (i.e. with WiFi still connected)
     */
    vbat_sample();
    PR02("VBat: %dmV, VTrigger: %dmV\n", vbat, vbat_trigger);

    // signal undervoltage indication after regular cmd if applicable
    // in the hope the server can already accept a new command (previous must be finished)
    if (vbat < vbat_trigger) {
        vbat_report(at_ptr, 1); // WiFi, end to end to KARR_TARGET_HOST -> statmsg can be trusted
    }
}   // if (!strcmp(at_ptr->host, KARR_TARGET_HOST))
#endif  // if defined(VBAT_ADC1_SENSE_PIN)
TPROF("vbat_block_done");

#if defined(LIGHT_SLEEP_TIMEOUT)
    /*
     * optionally here for energy efficiency:
     * if deep sleep is up and ahead we prematurely switch off wireless here
     * as traffic + battery voltage is already done
     */
    if (my_sleep_mode_is_deep) {
#endif
    wireless(0);
#if defined(LIGHT_SLEEP_TIMEOUT)
    }
#endif
TPROF("wireless_off");
// -----------------------------------------------------
out:
    wait_for_key_release(); // avoid looping through deep sleep
TPROF("key_released");

    //led_act(0);    // goes off by itself in sleep
    //rgb_off();    // must set this actively before sleep (if applicable)

    return 0;
}

_i32
resort_to_deep_sleep()
{
TP05
rgb_off();
    wireless(0);
TPROF("rts_wireless0");
    prepare_keys_for_deep_sleep(1);
TPROF("rts_prep_sleep");
    beep_sync();                // wait for all queued tones to be processed
TPROF("pre_sleep");
TPROF_DUMP();
    PR01("TP99: %lu\n", tstamp());
    esp_deep_sleep_start();     // won't return
    return 0;
}

#if defined(LIGHT_SLEEP_TIMEOUT)

#define ESP_INTR_FLAG_DEFAULT 0

QueueHandle_t key_evt_queue = 0;

void IRAM_ATTR
isr_key_queue(void *arg)
{
    _u32 io_num = (_u32)arg;
    xQueueSendFromISR(key_evt_queue, &io_num, 0);
}

gptimer_handle_t gptimer = 0;

#define LIGHT_SLEEP_CHECK_KEY   0xdead      // fake key to signal a light sleep timeout

_i32
extend_light_sleep_timeout()
{
TP05
    ESP_ERROR_CHECK(gptimer_stop(gptimer));
    ESP_ERROR_CHECK(gptimer_set_raw_count(gptimer, 0));     // reset counter back to zero
    ESP_ERROR_CHECK(gptimer_start(gptimer));                // start again -> timeout occurs LIGHT_SLEEP_TIMEOUT us later
    return 0;
}

//TODO???
#if 0
    PR05("Stop timer\n");
    ESP_ERROR_CHECK(gptimer_stop(gptimer));
    PR05("Disable timer\n");
    ESP_ERROR_CHECK(gptimer_disable(gptimer));
    PR05("Delete timer\n");
    ESP_ERROR_CHECK(gptimer_del_timer(gptimer));
#endif

bool IRAM_ATTR 
light_sleep_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;

    _u32 key = LIGHT_SLEEP_CHECK_KEY;
    xQueueSendFromISR(key_evt_queue, &key, &high_task_awoken);
    return high_task_awoken == pdTRUE;
}

_i32 
setup_light_sleep_timer(void)
{
TP05
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 1MHz, 1 tick=1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
    gptimer_event_callbacks_t cbs = {
        .on_alarm = light_sleep_timer_cb,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, (void *)LIGHT_SLEEP_CHECK_KEY));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    gptimer_alarm_config_t alarm_config1 = {
        .alarm_count = LIGHT_SLEEP_TIMEOUT,
        .flags.auto_reload_on_alarm = false,    // not required -> set to 0 autom. anyway
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config1));
    ESP_ERROR_CHECK(gptimer_start(gptimer));
    return 0;
}

void
dispatch_key_queue(void *arg)
{
TP05
    _u32 key;
    _u32 io_num;

    setup_light_sleep_timer();
    for (;;) {
        prepare_keys_for_light_sleep(1);
        PR04("dispatch: wait for key\n");
        if (xQueueReceive(key_evt_queue, &io_num, portMAX_DELAY)) {         // go to light sleep
            PR04("dispatch: received key_sns_x io_num: 0x%0x\n", io_num);
            prepare_keys_for_light_sleep(0);
            if (io_num == LIGHT_SLEEP_CHECK_KEY) {
                resort_to_deep_sleep(); // won't return
            }
            init_keys();
            // allow to retry to compensate for bouncing effects
            for (_u8 retry = 0; retry <= 5; ++retry) {
                key = scan_keys();
                if (key) break;
                PR00("key: %d -> retry %d/%d\n", key, retry + 1, 5);
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            PR04("key: %d (final)\n", key);
            // only account for real keys, ignore stray key triggers through weak pulls :-)
            if (key) {
                extend_light_sleep_timeout();
            }
            // will play sounds at least if stray (== 0) keys
            process_input(key);

            /*
             * check for sleep mode transition LIGHT -> DEEP (due to menu key)
             */
            if (my_sleep_mode_is_deep) {
                resort_to_deep_sleep(); // won't return
            }
            xQueueReset(key_evt_queue);
        }
    }
}

void
setup_key_queue()
{
TP05
    key_evt_queue = xQueueCreate(10, sizeof(_u32));
    xTaskCreate(dispatch_key_queue, "dispatch_key_queue", 4096, 0, 5, 0);
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT));
    /*
     * allow all SNS lines to interrupt
     */
    ESP_ERROR_CHECK(gpio_isr_handler_add(KEY_SNS_0, isr_key_queue, (void *)KEY_SNS_0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(KEY_SNS_1, isr_key_queue, (void *)KEY_SNS_1));
    ESP_ERROR_CHECK(gpio_isr_handler_add(KEY_SNS_2, isr_key_queue, (void *)KEY_SNS_2));
    ESP_ERROR_CHECK(gpio_isr_handler_add(KEY_SNS_3, isr_key_queue, (void *)KEY_SNS_3));
}

#endif  // defined(LIGHT_SLEEP_TIMEOUT)

void
app_main()
{
TP05
TPROF("app_main");
    PR01("TP00: %lu\n", tstamp());
    _u32 key;

#if defined(LIGHT_SLEEP_TIMEOUT)
    my_sleep_mode_is_deep = 1;          // getting here implies this
    PR06("implicitly setting sleep mode to: %s\n", my_sleep_mode_is_deep ? "deep" : "light");
#endif
    init_1st();
TPROF("init_1st");
    prepare_keys_for_deep_sleep(0);     // undo the work of preparing for deep sleep with active internal pull Rs
TPROF("prep_keys_undo");
    init_keys();
TPROF("init_keys");
    key = scan_keys();
TPROF("scan_keys");

    PR02("key: 0x%02x\n", key);
    init_2nd();
TPROF("init_2nd");
#if defined(VBAT_PATROL_INTERVAL) && !defined(DEBUG_TIMER_WAKEUP)
    /*
     * our own patrol timer fired, AKA nobody pressed anything. skip the whole key/menu machinery -
     * process_input(0) would issue its introducing spike beep, and that alone costs ~410ms of
     * beep_sync() in resort_to_deep_sleep() on the way back down, every single interval
     *
     * init_2nd() has to have run first: vbat_monitor_init() lives in there
     */
    /*
     * !defined(DEBUG_TIMER_WAKEUP) added 2026_09_15 to MATCH the arming site in
     * prepare_keys_for_deep_sleep(), which has always carried it. the two guards disagreeing was a
     * silent trap: the patrol timer was correctly never armed, but THIS test still caught every
     * timer wakeup - and DEBUG_TIMER_WAKEUP's wakeups are timer wakeups too. so with both defined
     * the patrol swallowed every simulated key press, "goto out" ran before process_input() ever
     * did, and the trace showed nothing but a 20ms patrol cycle once a second.
     *
     * the note at VBAT_PATROL_INTERVAL says "put DEBUG_TIMER_WAKEUP back and the patrol silently
     * disappears". it was the exact inverse - the patrol silently took over - which is worse,
     * because a profiling run then measures the patrol path and looks merely uneventful
     */
    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
        vbat_patrol();
TPROF("vbat_patrol");
        goto out;
    }
#endif
    if (key && bootCount == 1) {
        GET_NVS(pulsevol_NVS);
        _pulsevol = pulsevol_NVS ? pulsevol_NVS : _PULSEVOL;
        GET_NVS(pulsewidth_NVS);
        _pulsewidth = pulsewidth_NVS ? pulsewidth_NVS : _PULSEWIDTH;
        PR06(" ---vvv--- loading _ from NVS\n");
        PR06("pulsevol_NVS: %lx\n", pulsevol_NVS);
        PR06("pulsewidth_NVS: %lx\n", pulsewidth_NVS);
        PR06("_pulsevol: 0x%02x\n", _pulsevol);
        PR06("_pulsewidth: 0x%02x\n", _pulsewidth);
        PR06(" ---^^^--- loading _ from NVS\n");
        _u32 now = 0;
        while (tstamp() - now < MENU_SWITCH_TARGET_TIMEOUT) {
            interval_beep();
            vTaskDelay(pdMS_TO_TICKS(MENU_SWITCH_RESCAN_SPACING));
#if ESP32_(11) || ESP32_(34) || ESP32_(36) || ESP32_(70) || ESP32_(71) || ESP32_(72)
/*
 . . . .    00 na                   
 . . . 1    01 [ leave configuration ]
 . . 1 .    02 [ reset pulse         ]
 . . 1 1    03 na                   
 . 1 . .    04 [ decrease pulse vol  ]
 . 1 . 1    05 na                   
 . 1 1 .    06 na                   
 . 1 1 1    07 na                   
 1 . . .    08 [ increase pulse vol  ]
 1 . . 1    09 na                   
 1 . 1 .    10 na                   
 1 . 1 1    11 na                   
 1 1 . .    12 na                   
 1 1 . 1    13 na                   
 1 1 1 .    14 na                   
 1 1 1 1    15 na             
*/
            if (key = scan_keys()) {
                if (       key == 1) {
                    break;
                } else if (key == 2) {
                    _pulsevol = _PULSEVOL;
                    _pulsewidth = _PULSEWIDTH;
                } else if (key ==  4) {
                    _pulsevol -= 10;
                } else if (key ==  8) {
                    _pulsevol += 10;
                }
                _pulsevol &= 0x1fff;
                _pulsewidth &= 0x1ffff;
                now = tstamp();
            }
#else
/*
16  12          2a 2b  1000000000000000 0000100000000000    [ leave configuration  ]  [ reset pulse          ]
15  11          2c 2d  0100000000000000 0000010000000000    [                      ]  [                      ]
14  10          2e 2f  0010000000000000 0000001000000000    [                      ]  [                      ]
13   9  =>      2g 2h  0001000000000000 0000000100000000    [                      ]  [                      ]
8    4          2i 2j  0000000010000000 0000000000001000    [                      ]  [                      ]
7    3          2m 2n  0000000001000000 0000000000000100    [ decrease pulse width ]  [ increase pulse width ]
6    2          2o 2p  0000000000100000 0000000000000010    [ decrease pulse vol   ]  [ increase pulse vol   ]
*/
            if (key = scan_keys()) {
                if (       key == 16) {
                    break;
                } else if (key == 12) {
                    _pulsevol = _PULSEVOL;
                    _pulsewidth = _PULSEWIDTH;
                } else if (key == 15) {
                } else if (key == 11) {
                } else if (key == 14) {
                } else if (key == 10) {
                } else if (key == 13) {
                } else if (key ==  9) {
                } else if (key ==  8) {
                } else if (key ==  4) {
                } else if (key ==  7) {
                    _pulsewidth -= 100;
                } else if (key ==  3) {
                    _pulsewidth += 100;
                } else if (key ==  6) {
                    _pulsevol -= 10;
                } else if (key ==  2) {
                    _pulsevol += 10;
                }
                _pulsevol &= 0x1fff;
                _pulsewidth &= 0x1ffff;
                now = tstamp();
            }
#endif
        }

        PR06("---vvv--- save vals to NVS\n");
        PR06("pulsevol_NVS: 0x%02x\n", _pulsevol);
        PR06("pulsewidth_NVS: 0x%02x\n", _pulsewidth);
        PR06("---^^^--- save vals to NVS!\n");
        SET_NVS(pulsevol_NVS, _pulsevol);
        SET_NVS(pulsewidth_NVS, _pulsewidth);

        PR00("-------OTA-------\n");
        if (!ur_connect(OTA_SSID, WIFI_CONN_WAIT, WIFI_CONN_SLOW_FAIL, WIFI_PS_NONE)) {
            if (myota()) {          // as of 2026_03_15: returns anyway (on success and failure)
                PR00("OTA ended with error\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));    // grace period
        esp_restart();  // as a side effect resets bootCount to 1 for the next boot
                        // what happens if OTA were run successfully
                        // wouldn't reach this point in that case anyway
                        // we must do this to allow SSID and PASS being reprogrammed
                        // to standard values in case OTA did not take place and/or failed

    } else if (bootCount == 1) {
        /*
         * no key pressed here -> one-time-copy of configured NVS parameters into RTC
         */
        GET_NVS(pulsevol_NVS);
        pulsevol_RTC = pulsevol_NVS ? pulsevol_NVS : _PULSEVOL;
        GET_NVS(pulsewidth_NVS);
        pulsewidth_RTC = pulsewidth_NVS ? pulsewidth_NVS : _PULSEWIDTH;
        PR06("---vvv--- loading RTC from NVS\n");
        PR06("pulsevol_NVS: %lx\n", pulsevol_NVS);
        PR06("pulsewidth_NVS: %lx\n", pulsewidth_NVS);
        PR06("pulsevol_RTC: 0x%02x\n", pulsevol_RTC);
        PR06("pulsewidth_RTC: 0x%02x\n", pulsewidth_RTC);
        PR06("---^^^--- loading RTC from NVS\n");
        /*
         * getting here implies deep sleep
         * as we know no key is pressed -> skip process_input() and others
         */
        goto out;
    }
    _pulsevol = pulsevol_RTC;
    _pulsewidth = pulsewidth_RTC;
    PR06("---vvv--- loading regular from RTC\n");
    PR06("_pulsevol: 0x%02x\n", _pulsevol);
    PR06("_pulsewidth: 0x%02x\n", _pulsewidth);
    PR06("---^^^--- loading regular from RTC\n");

    /*
     * don't check for !key as we want to be notified by spike beep of stray keys
     */
    process_input(key);

    /*
     * check for sleep mode transition DEEP -> LIGHT (due to menu key)
     */
out:
#if defined(LIGHT_SLEEP_TIMEOUT)
    if (my_sleep_mode_is_deep) {
#endif  // if defined(LIGHT_SLEEP_TIMEOUT)

        resort_to_deep_sleep(); // won't return

#if defined(LIGHT_SLEEP_TIMEOUT)
    } else {
        setup_key_queue();

rgb_blue(1);
        /*
         * fall through -> exit app_main() here / as keyboard handling now is processed in dispatch_key_queue()
         */
    }
#endif  // if defined(LIGHT_SLEEP_TIMEOUT)

}

