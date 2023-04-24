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
#include "driver/rtc_io.h"

#if defined(ESP32_2)
#   define DEBUG    10
#elif defined(ESP32_12)
#   define DEBUG     1
//#   define BUZZER    2
#else
this may not happen
#endif

#include "mlcf.h"
#ifdef MCFG_LOCAL
#include "mcfg_local.h"
#include "mcfg_locals.h"
#else
#include "mcfg.h"
#endif
#include "mcom.h"

/* ---vvv--- KEY section ---vvv--- */
// sense keys pulled down by 100k 
// must be connected to RTC capable terminals to allow ESP_EXT1_WAKEUP_ANY_HIGH
#define KEY_SNS_0 27
#define KEY_SNS_1 26
#define KEY_SNS_2  4
#define KEY_SNS_3 14

#if defined(ESP32_2)
// fake sense key pulled down by 470k
#define KEY_FAKE_1 2
#define KEY_SNS_MASK (1 << KEY_FAKE_1)
#else
#define KEY_SNS_MASK (1 << KEY_SNS_0 | \
                      1 << KEY_SNS_1 | \
                      1 << KEY_SNS_2 | \
                      1 << KEY_SNS_3)
#endif

// feeder keys pulled up by 51k 
// sense keys connect to feeders on cross points
// when the corresponding key is depressed
// causing a 'high' level being detected to wakeup.
// after this the matrix is scanned.
// can optionally use RTC capable terminals (but with no benefit)
#define KEY_HOT_0  17  
#define KEY_HOT_1  16
#define KEY_HOT_2   5
#define KEY_HOT_3  25 

#define KEY_NONE 0xff

/*
scanned key codes to key caps ordering (hex):

 0  1      KEY_01 KEY_02     pause media
10 11      KEY_03 KEY_04     vol-  vol+
20 21      KEY_05 KEY_06     backw forw
30 31  =>  KEY_07 KEY_08     unfav fav
 2  3      KEY_09 KEY_10     undel del          
12 13      KEY_11 KEY_12     mute nvol      
22 23      KEY_13 KEY_14     rpt  opul
*/

#define KEY_01 0x00
#define KEY_02 0x01
#define KEY_03 0x10
#define KEY_04 0x11
#define KEY_05 0x20
#define KEY_06 0x21
#define KEY_07 0x30
#define KEY_08 0x31
#define KEY_09 0x02
#define KEY_10 0x03
#define KEY_11 0x12
#define KEY_12 0x13
#define KEY_13 0x22
#define KEY_14 0x23

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
exec_cmd(_u8 key)
{
TP05
    _i8p cmd = 0;

    if (key == KEY_01) {
        cmd = "zp";
    } else if (key == KEY_02) {
        cmd = "zm";
    } else if (key == KEY_03) {
        cmd = "zh";
    } else if (key == KEY_04) {
        cmd = "zk";
    } else if (key == KEY_05) {
        cmd = "zb";
    } else if (key == KEY_06) {
        cmd = "zn";
    } else if (key == KEY_07) {
        cmd = "zc";
    } else if (key == KEY_08) {
        cmd = "zx";
    } else if (key == KEY_09) {
        cmd = "zt";
    } else if (key == KEY_10) {
        cmd = "zr";
    } else if (key == KEY_11) {
        cmd = "zl";
    } else if (key == KEY_12) {
        cmd = "zj";
    } else if (key == KEY_13) {
        cmd = "zv";
    } else if (key == KEY_14) {
#if defined(ESP32_2)
        cmd = "@beep= f:1000 c:1 t:.05 p:.25 g:-20 ^roja ^";
#else
        cmd = "bz";     // available explicitly
#endif
    } else {
        PR05("illeg key: %02x\n", key);
    }
    if (cmd) {
        mysend(cmd, SMART_TARGET_HOST, SMART_TARGET_PORT, 0);
    }
}

gpio_config_t io_conf = {
    0,
    GPIO_MODE_INPUT,
    GPIO_PULLUP_DISABLE,
    GPIO_PULLDOWN_DISABLE,
    GPIO_INTR_DISABLE,
};

#if defined(ESP32_2)

void
init_matrix()
{
//TP05
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << KEY_FAKE_1;
    gpio_config(&io_conf);
}

_u8
scan_matrix()
{
//TP05
    if (gpio_get_level(KEY_FAKE_1)) {  // high if pressed
        return KEY_14;  // simulate beep
        return KEY_01;  // simulate pause
    } else {
        return KEY_NONE;
    }
}

#else

void
init_matrix()
{
//TP05
    _u8 h, s;

    io_conf.mode = GPIO_MODE_OUTPUT;
    for (h = 0; h < _NE(key_hot); ++h) {
        io_conf.pin_bit_mask = 1ULL << key_hot[h];
        gpio_config(&io_conf);
    }
    io_conf.mode = GPIO_MODE_INPUT;
    for (s = 0; s < _NE(key_sns); ++s) {
        io_conf.pin_bit_mask = 1ULL << key_sns[s];
        gpio_config(&io_conf);
    }
}

_u8
scan_matrix()
{
//TP05
    _u8 h, s, i, key;

    key = KEY_NONE;
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

    while ((key = scan_matrix()) != KEY_NONE) {
if (DEBUG > 5) PR05("C key: 0x%x\n", key);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        ++cnt;
    }
if (DEBUG > 1) PR05("key RELEASED\n");
    return cnt;
}
/* ---^^^--- KEY section ---^^^--- */

void
app_main()
{
//TP05
if (DEBUG) PR05("TP01: %lu\n", esp_log_timestamp());
    _u8 key;

    init_1st();
    init_matrix();
    key = scan_matrix();
if (DEBUG > 5) PR05("A key: 0x%x [ %lu ]\n", key, esp_log_timestamp());
    if (key != KEY_NONE && bootCount == 1) {
PR05("-------OTA-------\n");
        ESP_ERROR_CHECK(ur_connect(OTA_SSID));
        esp_wifi_set_ps(WIFI_PS_NONE);              // <== (RE)CHECK THIS
        if (!esp_https_ota(&ota_config)) {
            esp_restart();
        } else {
            PR05("could not OTA\n");
            key = KEY_NONE;                         // avoid executing further cmds
        }
        ESP_ERROR_CHECK(ur_disconnect());

    } else if (key == KEY_14) {
PR05("-------DOOR-------\n");
        _u32 last;

#if 0
        ESP_ERROR_CHECK(ur_connect(DOOR_SSID));
        if (mysend(DOOR_CMD_ASSERT, DOOR_TARGET_HOST, DOOR_TARGET_PORT, 0)) {
            PR05("could not assert signal\n");
            goto err;
        }
        last = wait_for_key_release();
        if (mysend(DOOR_CMD_DEASSERT, DOOR_TARGET_HOST, DOOR_TARGET_PORT, 0)) {
            PR05("could not deassert signal\n");
            goto err;
        }
        if (last > 4) {
            key = KEY_14;   // fake key for TETHER_OFF
        } else {
            key = KEY_NONE;
        }
#endif
    }
    if (key != KEY_NONE) {
PR05("-------SMARTPH-------\n");
//      ESP_ERROR_CHECK(ur_connect(SMART_SSID));
        ESP_ERROR_CHECK(ur_connect(ROTA2G_SSID));
if (DEBUG) PR05("TP02: %lu WiFi connected\n", esp_log_timestamp());
        exec_cmd(key);
        ESP_ERROR_CHECK(ur_disconnect());
    }
err:
    wait_for_key_release(); // avoid looping through deep sleep
    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(KEY_SNS_MASK, ESP_EXT1_WAKEUP_ANY_HIGH));
PR05("going to deep sleep\n");
    esp_deep_sleep_start();
}

