/*
 * code heavily based on 
 *
 *   * esp-idf/examples/system/startup_time at master · espressif/esp-idf
 *   https://github.com/espressif/esp-idf/tree/master/examples/system/startup_time
 * and
 *   * esp-idf/examples/protocols/http_request at master · espressif/esp-idf
 *   https://github.com/espressif/esp-idf/tree/master/examples/protocols/http_request
 * and
 *   * esp-idf/examples/system/ota/simple_ota_example at master · espressif/esp-idf
 *   https://github.com/espressif/esp-idf/tree/master/examples/system/ota/simple_ota_example
 *
 * thanks to the Espressif people for providing this
 */
#include "sdkconfig.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_sleep.h"
#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include "lwip/netdb.h"
#include "driver/ledc.h"
#include "driver/rtc_io.h"
#include "driver/gptimer.h"

#if defined(ESP32_2)            // --- development device ---
#   define DEBUG                 1
#   define BUZZER               23
#   define VBAT_ADC1_GND_PIN    27              // use a dynamic ground to effectively void its deep sleep current
#   define VBAT_ADC1_SENSE_PIN  ADC_CHANNEL_4   // IO32

#if 0
#if 0
#define FIRST_SSID          DOOR_SSID
#else
#define FIRST_SSID          ROTA2G_SSID
#endif
#define FIRST_TARGET_HOST   DOOR_TARGET_HOST
#define FIRST_TARGET_PORT   DOOR_TARGET_PORT
#define FIRST_CMD_ASSERT    DOOR_CMD_OPL
#define FIRST_CMD_DEASSERT  DOOR_CMD_OPLoff
#endif

#if 1
#if 0
#define SECND_SSID          SMART_SSID
#define SECND_TARGET_HOST   SMART_TARGET_HOST
#define SECND_TARGET_PORT   SMART_TARGET_PORT
#else
#define SECND_SSID          ROTA2G_SSID
#define SECND_TARGET_HOST   STD_TARGET_HOST
#define SECND_TARGET_PORT   STD_TARGET_PORT
#endif
#endif

#elif defined(ESP32_9)          // --- 1 button remote control ---
#   define DEBUG                 0
#   define BUZZER               32
//#   define VBAT_ADC1_GND_PIN    27
//#   define VBAT_ADC1_SENSE_PIN  ADC_CHANNEL_4

#define FIRST_SSID          ROTA2G_SSID
#define FIRST_TARGET_HOST   DOOR_TARGET_HOST
#define FIRST_TARGET_PORT   DOOR_TARGET_PORT
#define FIRST_CMD_ASSERT    DOOR_CMD_OPL
#define FIRST_CMD_DEASSERT  DOOR_CMD_OPLoff

#elif defined(ESP32_10)         // --- 16 button remote control ---
#   define DEBUG                 0
#   define BUZZER                2
//#   define VBAT_ADC1_GND_PIN    27
//#   define VBAT_ADC1_SENSE_PIN  ADC_CHANNEL_4

#define SECND_SSID          ROTA2G_SSID
#define SECND_TARGET_HOST   STD_TARGET_HOST
#define SECND_TARGET_PORT   STD_TARGET_PORT

#elif defined(ESP32_11)         // --- 4 button remote control ---
#   define DEBUG                 0
//#   define BUZZER                2
//#   define VBAT_ADC1_GND_PIN    27
//#   define VBAT_ADC1_SENSE_PIN  ADC_CHANNEL_4

#define FIRST_SSID          ROTA2G_SSID
#define FIRST_TARGET_HOST   DOOR_TARGET_HOST
#define FIRST_TARGET_PORT   DOOR_TARGET_PORT
#define FIRST_CMD_ASSERT    DOOR_CMD_OPL
#define FIRST_CMD_DEASSERT  DOOR_CMD_OPLoff

#elif defined(ESP32_12)         // --- 14 button remote control ---
#   define DEBUG                 0
//#   define BUZZER                2
//#   define VBAT_ADC1_GND_PIN    27
//#   define VBAT_ADC1_SENSE_PIN  ADC_CHANNEL_4

#define FIRST_SSID          DOOR_SSID
#define FIRST_TARGET_HOST   DOOR_TARGET_HOST
#define FIRST_TARGET_PORT   DOOR_TARGET_PORT

#define SECND_SSID          SMART_SSID
#define SECND_TARGET_HOST   SMART_TARGET_HOST
#define SECND_TARGET_PORT   SMART_TARGET_PORT
#define FIRST_CMD_ASSERT    DOOR_CMD_OPUL
#define FIRST_CMD_DEASSERT  DOOR_CMD_OPLoff

#else
#error: no ESP32_x device defined
#endif

#include "mlcf.h"
#if DEBUG < 5
    #undef TP05
    #define TP05
#endif
#ifdef MCFG_LOCAL
#include "mcfg_local.h"
#include "mcfg_locals.h"
#else
#include "mcfg.h"
#endif
#include "mcom.h"

/* ---vvv--- KEY section ---vvv--- */

#define KEY_CODE_NONE 0xff

#if defined(ESP32_2)                        // --- development device ---

// single sense key for test device pulled down by external 470k
// must be connected to RTC capable terminals to allow ESP_EXT1_WAKEUP_ANY_HIGH
#define KEY_SNS 2
#define KEY_SNS_MASK (1 << KEY_SNS)

#if defined(FIRST_TARGET_HOST)
#define KEY_RAW_FIRST_CMD 0
#endif

#if defined(SECND_TARGET_HOST)

/*
scanned key codes to key caps ordering:

00  =>  test

*/
_i8p
key_raw2cmd[] = {
#if SECND_SSID == ROTA2G_SSID
      _w1("@beep= f:1000 c:1 t:.05 p:.25 g:-20 ^roja ^")
#elif SECND_SSID == SMART_SSID
//    _w1("bz")
    _w1("zp")
//    SMART_CMD_TETHER_OFF_DEL
#endif
};
#endif

void
init_keys()
{
TP05
    static gpio_config_t io_conf = {};

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << KEY_SNS;
    gpio_config(&io_conf);
}

_u8
scan_keys()
{
TP05
    _u8 ret = gpio_get_level(KEY_SNS);  // high if pressed

    return ret ? !ret : KEY_CODE_NONE;
}

#elif defined(ESP32_9)                      // --- 1 button remote control ---

// single sense key with internal pullup in use
// must be connected to a RTC capable terminal to allow use of esp_sleep_enable_ext0_wakeup() 
#define KEY_SNS 4

#if defined(FIRST_TARGET_HOST)
#define KEY_RAW_FIRST_CMD 0
#endif

#if defined(SECND_TARGET_HOST)

#does not apply ATM

/*
scanned key codes to key caps ordering:

00  =>  fct

*/
_i8p
key_raw2cmd[] = {
};
#endif

void
init_keys()
{
TP05
    static gpio_config_t io_conf = {};

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << KEY_SNS;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;  // experimental: use the builtin pull as it appears to have no impact on deep sleep current
    gpio_config(&io_conf);
}

_u8
scan_keys()
{
TP05
    _u8 ret = !gpio_get_level(KEY_SNS);  // low if key pressed

    return ret ? !ret : KEY_CODE_NONE;
}

#elif defined(ESP32_10)                     // --- 16 button remote control ---

// sense keys connect to feeders on cross points
// when the corresponding key is depressed.
// causing a 'high' level being detected to wakeup.
// after this the matrix is scanned.
// feeders can optionally use RTC capable terminals (but with no benefit)

// hot (feeding) keys pulled up by external 51k 
#define KEY_HOT_0 33
#define KEY_HOT_1 32
#define KEY_HOT_2  4 
#define KEY_HOT_3 12

// sense keys pulled down by external 100k 
// must be connected to RTC capable terminals to allow ESP_EXT1_WAKEUP_ANY_HIGH
#define KEY_SNS_0 25
#define KEY_SNS_1 26
#define KEY_SNS_2 27
#define KEY_SNS_3 14

#define KEY_SNS_MASK (1 << KEY_SNS_0 | \
                      1 << KEY_SNS_1 | \
                      1 << KEY_SNS_2 | \
                      1 << KEY_SNS_3)

#if defined(FIRST_TARGET_HOST)
#does not apply
#endif

#if defined(SECND_TARGET_HOST)

/*
scanned key codes to key caps ordering:

00 10      pause            media
01 11      vol-             vol+
02 12      backw            forw
03 13  =>  unfav            fav
20 30      undel            del
21 31      all light off    repeat
22 32      ev light bd_b4   vol mid
23 33      ev light dim_b3  amps audio

*/
_i8p
key_raw2cmd[] = {
    _w1("zp ^"),                // 0 << 2 | 0   00
    _w1("@vol=5%- ^roja ^"),    // 0 << 2 | 1   01
    _w1("zb ^"),                // 0 << 2 | 2   02
    _w1("zc ^"),                // 0 << 2 | 3   03

    _w1("zm ^"),                // 1 << 2 | 0   04
    _w1("@vol=5%+ ^roja ^"),    // 1 << 2 | 1   05
    _w1("zn ^"),                // 1 << 2 | 2   06
    _w1("zx ^"),                // 1 << 2 | 3   07

    _w1("zt ^"),                // 2 << 2 | 0   08
    _w1("cq ^"),                // 2 << 2 | 1   09
    _w1("cu ^"),                // 2 << 2 | 2   10
    _w1("cy ^"),                // 2 << 2 | 3   11

    _w1("zr ^"),                // 3 << 2 | 0   12
    _w1("zv ^"),                // 3 << 2 | 1   13
    _w1("@vol=60% ^roja ^"),    // 3 << 2 | 2   14
    _w1("xc ^"),                // 3 << 2 | 3   15
};
#endif

#elif defined(ESP32_11)                     // --- 4 button remote control ---

// 4 sense keys pulled down by external 470k
// must be connected to RTC capable terminals to allow ESP_EXT1_WAKEUP_ANY_HIGH
#define KEY_SNS_0 12
#define KEY_SNS_1 14
#define KEY_SNS_2 27
#define KEY_SNS_3 26

#define KEY_SNS_MASK (1 << KEY_SNS_0 | \
                      1 << KEY_SNS_1 | \
                      1 << KEY_SNS_2 | \
                      1 << KEY_SNS_3)

#if defined(FIRST_TARGET_HOST)
#define KEY_RAW_FIRST_CMD 0
#endif

#if defined(SECND_TARGET_HOST)

#does not apply ATM

/*
scanned key codes to key caps ordering:

00  =>  fct

*/
_i8p
key_raw2cmd[] = {
    "",             // . . . .    00
    "zp",           // . . . 1    01
    "zh",           // . . 1 .    02
    "ae ^dim ^",    // . . 1 1    03
    "zk",           // . 1 . .    04
    "ae ^dim ^",    // . 1 . 1    05
    "ae ^dim ^",    // . 1 1 .    06
    "ae ^dim ^",    // . 1 1 1    07
    "zm",           // 1 . . .    08
    "ae ^dim ^",    // 1 . . 1    09
    "ae ^dim ^",    // 1 . 1 .    10
    "ae ^dim ^",    // 1 . 1 1    11
    "ae ^dim ^",    // 1 1 . .    12
    "ae ^dim ^",    // 1 1 . 1    13
    "ae ^dim ^",    // 1 1 1 .    14
    "ae ^dim ^",    // 1 1 1 1    15
};
#endif

void
init_keys()
{
TP05
    static gpio_config_t io_conf = {};

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << KEY_SNS_0 
                         | 1ULL << KEY_SNS_1
                         | 1ULL << KEY_SNS_2
                         | 1ULL << KEY_SNS_3;
    gpio_config(&io_conf);
}

_u8
scan_keys()
{
TP05
    _u8 ret = 0;

    ret = ret << 1 | gpio_get_level(KEY_SNS_3);
    ret = ret << 1 | gpio_get_level(KEY_SNS_2);
    ret = ret << 1 | gpio_get_level(KEY_SNS_1);
    ret = ret << 1 | gpio_get_level(KEY_SNS_0);

//  return ret ? ret : KEY_CODE_NONE;
    return ret ? !ret : KEY_CODE_NONE; // map all keys to one ATM
}

#elif defined(ESP32_12)                     // --- 14 button remote control ---

// hot (feeding) keys pulled up by external 51k
#define KEY_HOT_0 17  
#define KEY_HOT_1 16
#define KEY_HOT_2  5
#define KEY_HOT_3 25 

// sense keys pulled down by external 100k
// must be connected to RTC capable terminals to allow ESP_EXT1_WAKEUP_ANY_HIGH
#define KEY_SNS_0 27
#define KEY_SNS_1 26
#define KEY_SNS_2  4
#define KEY_SNS_3 14

#define KEY_SNS_MASK (1 << KEY_SNS_0 | \
                      1 << KEY_SNS_1 | \
                      1 << KEY_SNS_2 | \
                      1 << KEY_SNS_3)

#if defined(FIRST_TARGET_HOST)
#define KEY_RAW_FIRST_CMD (2 << 2 | 3) // 11
#endif

#if defined(SECND_TARGET_HOST)

/*
scanned key codes to key caps ordering:

00 01      pause media      
10 11      vol-  vol+        
20 21      backw forw                            
30 31  =>  unfav fav   
02 03      undel del                                      
12 13      mute  nvol                                  
22 23      rpt   opul                            

*/
_i8p
key_raw2cmd[] = {
    _w1("zp"),                  // 0 << 2 | 0   00
    _w1("zm"),                  // 0 << 2 | 1   01
    _w1("zt"),                  // 0 << 2 | 2   02
    _w1("zr"),                  // 0 << 2 | 3   03

    _w1("zh"),                  // 1 << 2 | 0   04
    _w1("zk"),                  // 1 << 2 | 1   05
    _w1("zl"),                  // 1 << 2 | 2   06
    _w1("zj"),                  // 1 << 2 | 3   07

    _w1("zb"),                  // 2 << 2 | 0   08
    _w1("zn"),                  // 2 << 2 | 1   09
    _w1("zv"),                  // 2 << 2 | 2   10
    SMART_CMD_TETHER_OFF_DEL,   // 2 << 2 | 3   11

    _w1("zc"),                  // 3 << 2 | 0   12
    _w1("zx"),                  // 3 << 2 | 1   13
};
#endif

#endif

// these are handled the same
#if defined(ESP32_10) || \
    defined(ESP32_12)               // --- 16 button remote control ---
                                    // --- 14 button remote control ---
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

void
init_keys()
{
TP05
    _u8 h, s;
    static gpio_config_t io_conf = {};

    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 0;
    for (h = 0; h < _NE(key_hot); ++h) {
        io_conf.pin_bit_mask |= 1ULL << key_hot[h];
    }
    gpio_config(&io_conf);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 0;
    for (s = 0; s < _NE(key_sns); ++s) {
        io_conf.pin_bit_mask |= 1ULL << key_sns[s];
    }
    gpio_config(&io_conf);
}

_u8
scan_keys()
{
TP05
    _u8 h, s, i, key;

    key = KEY_CODE_NONE;
    for (h = 0; h < _NE(key_hot); ++h) {
        for (i = 0; i < _NE(key_hot); ++i) {
            gpio_set_level(key_hot[i], h == i);
        }
        for (s = 0; s < _NE(key_sns); ++s) {
            if (gpio_get_level(key_sns[s])) {
                key = h << 2 | s;
                goto end;
            }
        }
    }
end:
    return key;
}
#endif

_u32
wait_for_key_release()
{
TP05
    _u8 key;
    _u32 cnt = 0;

    while ((key = scan_keys()) != KEY_CODE_NONE) {
#if DEBUG > 5
        PR05("key: 0x%x\n", key);
#endif
        vTaskDelay(100 / portTICK_PERIOD_MS);
        ++cnt;
    }
#if DEBUG > 1
    PR05("key RELEASED after [ %d ]\n", cnt);
#endif
    return cnt;
}
/* ---^^^--- KEY section ---^^^--- */

void
app_main()
{
TP05
#if DEBUG
    PR05("TP01: %lu\n", esp_log_timestamp());
#endif
    _u8 key;

    init_1st();
    init_keys();
    key = scan_keys();
#if DEBUG > 5 
    PR05("key: 0x%x [ %lu ]\n", key, esp_log_timestamp());
#endif
    init_2nd();
#if defined(VBAT_ADC1_GND_PIN)
#if DEBUG
    _i32 mV;
    ESP_ERROR_CHECK(adc_oneshot_get_calibrated_result(adc1_unit_ptr, adc1_cali_ptr, VBAT_ADC1_SENSE_PIN, &mV));
    PR05("VBat: %dmV\n", mV);
#endif
#endif
// -----------------------------------------------------
    if (key == KEY_CODE_NONE) {
        goto out1;
// -----------------------------------------------------
    } else if (bootCount == 1) {
PR05("-------OTA-------\n");
        if (ur_connect(OTA_SSID, 1)) {
            PR05("could not connect to %s\n", GET_SSID(OTA_SSID));
            goto out1;
        }
        esp_wifi_set_ps(WIFI_PS_NONE);              // <== (RE)CHECK THIS
        beep(BEEP_OTA, 5);
        beep_sync();            // to stay on the safe side sync queues before entering OTA
        if (!esp_https_ota(&ota_config)) {
            PR05("OTA completed\n");
            beep(BEEP_OK, 1);
            beep_sync();
            esp_restart();
        } else {
            ESP_ERROR_CHECK(ur_disconnect());
            PR05("could not OTA\n");
            beep(BEEP_ERR, 3);
            goto out1;             
        }
// -----------------------------------------------------
#if defined(FIRST_TARGET_HOST)
    } else if (key == KEY_RAW_FIRST_CMD) {
#if DEBUG > 5
        PR05("-------FIRST-------\n");
#endif
        _u32 last;

        if (ur_connect(FIRST_SSID, 0)) {
            PR05("could not connect to %s\n", GET_SSID(FIRST_SSID));
            goto out1;
        }
        esp_wifi_set_ps(WIFI_PS_NONE);              // <== (RE)CHECK THIS
        if (mysend(FIRST_CMD_ASSERT, FIRST_TARGET_HOST, FIRST_TARGET_PORT, 0)) {
            PR05("could not assert signal\n");
            ESP_ERROR_CHECK(ur_disconnect());
            goto out1;
        }
        last = wait_for_key_release();
        if (mysend(FIRST_CMD_DEASSERT, FIRST_TARGET_HOST, FIRST_TARGET_PORT, 0)) {
            PR05("could not deassert signal\n");
            ESP_ERROR_CHECK(ur_disconnect());
            goto out1;
        }
        if (last <= 4) {
            ESP_ERROR_CHECK(ur_disconnect());
            goto out1; // switch tether off only for DOOR SIGNAL exceeding a minimum time interval 
        }
        ESP_ERROR_CHECK(ur_disconnect());
#endif
    }
// -----------------------------------------------------
#if defined(SECND_TARGET_HOST)
#if DEBUG > 5
    PR05("-------SECND-------\n");
#endif
    if (ur_connect(SECND_SSID, 0)) {
        PR05("could not connect to %s\n", GET_SSID(SECND_SSID));
        goto out1;
    }
    esp_wifi_set_ps(WIFI_PS_NONE);              // <== (RE)CHECK THIS
    mysend(key_raw2cmd[key], SECND_TARGET_HOST, SECND_TARGET_PORT, 0);
    ESP_ERROR_CHECK(ur_disconnect());
// -----------------------------------------------------
#endif
out1:
    wait_for_key_release(); // avoid looping through deep sleep
    beep_sync();  // wait for all queued tones to be processed 
#if defined(ESP32_9)            // --- 1 button remote control ---
    ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(KEY_SNS, 0));     // 1 = High, 0 = Low     
#else                           // --- all others ---
    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(KEY_SNS_MASK, ESP_EXT1_WAKEUP_ANY_HIGH));
#endif
#if DEBUG > 5
    PR05("TP14: %lu\n", esp_log_timestamp());
#endif
    esp_deep_sleep_start();
}

