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
#define KEY_CODE_14 0x03

#if defined(SECND_TARGET_HOST)
void
exec_cmd(_u8 key)
{
TP05
    _i8p cmd = 0;

    if (key == KEY_CODE_14) { // key may have caused other activities before, but required SSID now already in place
#if SECND_SSID == ROTA2G_SSID
        cmd = _w1("@beep= f:1000 c:1 t:.05 p:.25 g:-20 ^roja ^");
#elif SECND_SSID == SMART_SSID
//      cmd = _w1("bz"); 
        cmd = _w1("zp");
//      cmd = SMART_CMD_TETHER_OFF_DEL;
#endif
    } else {
        PR05("illeg key: %02x\n", key);
    }
    if (cmd) {
        if (mysend(cmd, SECND_TARGET_HOST, SECND_TARGET_PORT, 0)) {
            PR05("cmd failed\n");
        }
    }
}
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
    return gpio_get_level(KEY_SNS) ? KEY_CODE_14 : KEY_CODE_NONE; // high if pressed
}

#elif defined(ESP32_9)                      // --- 1 button remote control ---

// single sense key with internal pullup in use
// must be connected to a RTC capable terminal to allow use of esp_sleep_enable_ext0_wakeup() 
#define KEY_SNS 4
#define KEY_CODE_14 0x03

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
    return !gpio_get_level(KEY_SNS) ? KEY_CODE_14 : KEY_CODE_NONE;  // low if key pressed
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
/*
00 10    KEY_CODE_11 KEY_CODE_01       "zp ^"                  "zm ^"
01 11    KEY_CODE_12 KEY_CODE_02       "@vol=5%- ^roja ^"      "@vol=5%+ ^roja ^"
02 12    KEY_CODE_13 KEY_CODE_03       "zb ^"                  "zn ^"
03 13 => KEY_CODE_14 KEY_CODE_04       "zc ^"                  "zx ^"
20 30    KEY_CODE_15 KEY_CODE_05       "zt ^"                  "zr ^"
21 31    KEY_CODE_16 KEY_CODE_06       "cq ^"                  "zv ^"
22 32    KEY_CODE_17 KEY_CODE_07       "cu ^"                  "@vol=60% ^roja ^"
23 33    KEY_CODE_18 KEY_CODE_08       "cy ^"                  "xc ^"
*/
#define KEY_CODE_01 0x10
#define KEY_CODE_02 0x11
#define KEY_CODE_03 0x12
#define KEY_CODE_04 0x13
#define KEY_CODE_05 0x30
#define KEY_CODE_06 0x31
#define KEY_CODE_07 0x32
#define KEY_CODE_08 0x33
#define KEY_CODE_11 0x00
#define KEY_CODE_12 0x01
#define KEY_CODE_13 0x02
#define KEY_CODE_14 0x03
#define KEY_CODE_15 0x20
#define KEY_CODE_16 0x21
#define KEY_CODE_17 0x22
#define KEY_CODE_18 0x23

#if defined(SECND_TARGET_HOST)
void
exec_cmd(_u8 key)
{
TP05
    _i8p cmd = 0;

    if (key == KEY_CODE_11) {
        cmd = _w1("zp ^");
    } else if (key == KEY_CODE_01) {
        cmd = _w1("zm ^");
    } else if (key == KEY_CODE_12) {
//        cmd = _w1("zh ^");
        cmd = _w1("@vol=5%- ^roja ^");
    } else if (key == KEY_CODE_02) {
//        cmd = _w1("zk ^");
        cmd = _w1("@vol=5%+ ^roja ^");
    } else if (key == KEY_CODE_13) {
        cmd = _w1("zb ^");
    } else if (key == KEY_CODE_03) {
        cmd = _w1("zn ^");
    } else if (key == KEY_CODE_14) {
        cmd = _w1("zc ^");
    } else if (key == KEY_CODE_04) {
        cmd = _w1("zx ^");
    } else if (key == KEY_CODE_15) {
        cmd = _w1("zt ^");
    } else if (key == KEY_CODE_05) {
        cmd = _w1("zr ^");
    } else if (key == KEY_CODE_16) {
        cmd = _w1("cq ^");
    } else if (key == KEY_CODE_06) {
        cmd = _w1("zv ^");
    } else if (key == KEY_CODE_17) {
        cmd = _w1("cu ^");
    } else if (key == KEY_CODE_07) {
//        cmd = _w1("zj ^");
        cmd = _w1("@vol=60% ^roja ^");
    } else if (key == KEY_CODE_18) {
        cmd = _w1("cy ^");
    } else if (key == KEY_CODE_08) {
        cmd = _w1("xc ^");
    } else {
        PR05("illeg key: %02x\n", key);
    }
    if (cmd) {
        if (mysend(cmd, SECND_TARGET_HOST, SECND_TARGET_PORT, 0)) {
            PR05("cmd failed\n");
        }
    }
}
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

#define KEY_CODE_14 0x03

#ifdef __NOT_IN_USE__
_u8p cmds[] = {
    "",             // 00 . . . .   n/a
    "zp",           // 01 . . . 1 
    "zh",           // 02 . . 1 . 
    "ae ^dim ^",    // 03 . . 1 1 
    "zk",           // 04 . 1 . . 
    "ae ^dim ^",    // 05 . 1 . 1 
    "ae ^dim ^",    // 06 . 1 1 . 
    "ae ^dim ^",    // 07 . 1 1 1 
    "zm",           // 08 1 . . . 
    "ae ^dim ^",    // 09 1 . . 1 
    "ae ^dim ^",    // 10 1 . 1 . 
    "ae ^dim ^",    // 11 1 . 1 1 
    "ae ^dim ^",    // 12 1 1 . . 
    "ae ^dim ^",    // 13 1 1 . 1 
    "ae ^dim ^",    // 14 1 1 1 . 
    "ae ^dim ^",    // 15 1 1 1 1 
};

_i32
exec_cmd(_u8 keys)
{
TP05
if (DEBUG > 5) Serial.printf("exec cmd: 0x%x -> %s\n", keys, cmds[keys]);
    return mysend(cmds[keys], TARGET_HOST, TARGET_PORT, 0);
}
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
    _u8 _keys = 0;

    _keys = _keys << 1 | gpio_get_level(KEY_SNS_3);
    _keys = _keys << 1 | gpio_get_level(KEY_SNS_2);
    _keys = _keys << 1 | gpio_get_level(KEY_SNS_1);
    _keys = _keys << 1 | gpio_get_level(KEY_SNS_0);
    return _keys ? KEY_CODE_14 : KEY_CODE_NONE;
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
/*
scanned key codes to key caps ordering (hex):

 0  1      KEY_CODE_01 KEY_CODE_02     pause media
10 11      KEY_CODE_03 KEY_CODE_04     vol-  vol+
20 21      KEY_CODE_05 KEY_CODE_06     backw forw
30 31  =>  KEY_CODE_07 KEY_CODE_08     unfav fav
 2  3      KEY_CODE_09 KEY_CODE_10     undel del          
12 13      KEY_CODE_11 KEY_CODE_12     mute nvol      
22 23      KEY_CODE_13 KEY_CODE_14     rpt  opul
*/

#define KEY_CODE_01 0x00
#define KEY_CODE_02 0x01
#define KEY_CODE_03 0x10
#define KEY_CODE_04 0x11
#define KEY_CODE_05 0x20
#define KEY_CODE_06 0x21
#define KEY_CODE_07 0x30
#define KEY_CODE_08 0x31
#define KEY_CODE_09 0x02
#define KEY_CODE_10 0x03
#define KEY_CODE_11 0x12
#define KEY_CODE_12 0x13
#define KEY_CODE_13 0x22
#define KEY_CODE_14 0x23

#if defined(SECND_TARGET_HOST)
void
exec_cmd(_u8 key)
{
TP05
    _i8p cmd = 0;

    if (key == KEY_CODE_01) {
        cmd = _w1("zp");
    } else if (key == KEY_CODE_02) {
        cmd = _w1("zm");
    } else if (key == KEY_CODE_03) {
        cmd = _w1("zh");
    } else if (key == KEY_CODE_04) {
        cmd = _w1("zk");
    } else if (key == KEY_CODE_05) {
        cmd = _w1("zb");
    } else if (key == KEY_CODE_06) {
        cmd = _w1("zn");
    } else if (key == KEY_CODE_07) {
        cmd = _w1("zc");
    } else if (key == KEY_CODE_08) {
        cmd = _w1("zx");
    } else if (key == KEY_CODE_09) {
        cmd = _w1("zt");
    } else if (key == KEY_CODE_10) {
        cmd = _w1("zr");
    } else if (key == KEY_CODE_11) {
        cmd = _w1("zl");
    } else if (key == KEY_CODE_12) {
        cmd = _w1("zj");
    } else if (key == KEY_CODE_13) {
        cmd = _w1("zv");
    } else if (key == KEY_CODE_14) { // key may have caused other activities before, but required SSID now already in place
        cmd = SMART_CMD_TETHER_OFF_DEL;
    } else {
        PR05("illeg key: %02x\n", key);
    }
    if (cmd) {
        if (mysend(cmd, SECND_TARGET_HOST, SECND_TARGET_PORT, 0)) {
            PR05("cmd failed\n");
        }
    }
}
#endif

#endif

// these are handled the same
#if defined(ESP32_10) || \
      defined(ESP32_12)             // --- 16 button remote control ---
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
                key = h << 4 | s;
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
#if defined(FIRST_SSID)
    } else if (key == KEY_CODE_14) {
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
#if defined(SECND_SSID)
#if DEBUG > 5
    PR05("-------SECND-------\n");
#endif
    if (ur_connect(SECND_SSID, 0)) {
        PR05("could not connect to %s\n", GET_SSID(SECND_SSID));
        goto out1;
    }
    esp_wifi_set_ps(WIFI_PS_NONE);              // <== (RE)CHECK THIS
    exec_cmd(key);
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

