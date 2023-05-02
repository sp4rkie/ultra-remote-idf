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

#if defined(ESP32_2)
#   define DEBUG    0
#   define BUZZER  23

//  V1  V2
//  0   0   trigger dual route assert (DOOR) + tether-off (SMART) == standard
//  0   1   trigger dual route assert (ROTA2G) + pause (SMART)
//  1   0   trigger single route beep (ROTA2G)
//  1   1   trigger single route pause (SMART)
//#   define KEY_OVERRIDE_V1    
//#   define KEY_OVERRIDE_V2   

#elif defined(ESP32_10)
#   define DEBUG    0
#   define BUZZER   2

#elif defined(ESP32_12)
#   define DEBUG    0
//#   define BUZZER   2

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
// feeder (hot) keys pulled up by 51k 

// sense keys pulled down by 100k 
// must be connected to RTC capable terminals to allow ESP_EXT1_WAKEUP_ANY_HIGH

// sense keys connect to feeders on cross points
// when the corresponding key is depressed.
// causing a 'high' level being detected to wakeup.
// after this the matrix is scanned.
// can optionally use RTC capable terminals (but with no benefit)

#if defined(ESP32_2)

// fake (single) sense key for test device pulled down by 470k
#define KEY_FAKE_1 2
#define KEY_SNS_MASK (1 << KEY_FAKE_1)
#define KEY_14 0x03

void
exec_cmd(_u8 key)
{
TP05
    _i8p cmd = 0;

    if (key == KEY_14) { // key may have caused other activities before, but required SSID now already in place
#if defined(KEY_OVERRIDE_V1) && !defined(KEY_OVERRIDE_V2)
//      cmd = _w1("bz");             // err beep
        cmd = _w1("@beep= f:1000 c:1 t:.05 p:.25 g:-20 ^roja ^");
#elif defined(KEY_OVERRIDE_V2)
        cmd = _w1("zp");
#else
        cmd = SMART_CMD_TETHER_OFF_DEL;
#endif
    } else {
        PR05("illeg key: %02x\n", key);
    }
    if (cmd) {
        if (mysend(cmd, SMART_TARGET_HOST, SMART_TARGET_PORT, 0)) {
            PR05("cmd failed\n");
        }
    }
}

#elif defined(ESP32_10)

#define KEY_HOT_0 33
#define KEY_HOT_1 32
#define KEY_HOT_2  4 
#define KEY_HOT_3 12

#define KEY_SNS_0 25
#define KEY_SNS_1 26
#define KEY_SNS_2 27
#define KEY_SNS_3 14

#define KEY_SNS_MASK (1 << KEY_SNS_0 | \
                      1 << KEY_SNS_1 | \
                      1 << KEY_SNS_2 | \
                      1 << KEY_SNS_3)
/*
00 10
01 11
02 12
03 13
20 30
21 31
22 32
23 33
*/
#define KEY_01 0x10
#define KEY_02 0x11
#define KEY_03 0x12
#define KEY_04 0x13
#define KEY_05 0x30
#define KEY_06 0x31
#define KEY_07 0x32
#define KEY_08 0x33
#define KEY_11 0x00
#define KEY_12 0x01
#define KEY_13 0x02
#define KEY_14 0x03
#define KEY_15 0x20
#define KEY_16 0x21
#define KEY_17 0x22
#define KEY_18 0x23

void
exec_cmd(_u8 key)
{
    _i8p cmd = 0;

    if (key == KEY_11) {
        cmd = _w1("zp ^");
    } else if (key == KEY_01) {
        cmd = _w1("zm ^");
    } else if (key == KEY_12) {
//        cmd = _w1("zh ^");
        cmd = _w1("@vol=5%- ^roja ^");
    } else if (key == KEY_02) {
//        cmd = _w1("zk ^");
        cmd = _w1("@vol=5%+ ^roja ^");
    } else if (key == KEY_13) {
        cmd = _w1("zb ^");
    } else if (key == KEY_03) {
        cmd = _w1("zn ^");
    } else if (key == KEY_14) {
        cmd = _w1("zc ^");
    } else if (key == KEY_04) {
        cmd = _w1("zx ^");
    } else if (key == KEY_15) {
        cmd = _w1("zt ^");
    } else if (key == KEY_05) {
        cmd = _w1("zr ^");
    } else if (key == KEY_16) {
        cmd = _w1("cq ^");
    } else if (key == KEY_06) {
        cmd = _w1("zv ^");
    } else if (key == KEY_17) {
        cmd = _w1("cu ^");
    } else if (key == KEY_07) {
//        cmd = _w1("zj ^");
        cmd = _w1("@vol=60% ^roja ^");
    } else if (key == KEY_18) {
        cmd = _w1("cy ^");
    } else if (key == KEY_08) {
        cmd = _w1("xc ^");
    } else {
        PR05("illeg key: %02x\n", key);
    }
    if (cmd) {
        if (mysend(cmd, TARGET_HOST, TARGET_PORT, 0)) {
            PR05("cmd failed\n");
        }
    }
}

#elif defined(ESP32_12)

#define KEY_HOT_0 17  
#define KEY_HOT_1 16
#define KEY_HOT_2  5
#define KEY_HOT_3 25 

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

void
exec_cmd(_u8 key)
{
TP05
    _i8p cmd = 0;

    if (key == KEY_01) {
        cmd = _w1("zp");
    } else if (key == KEY_02) {
        cmd = _w1("zm");
    } else if (key == KEY_03) {
        cmd = _w1("zh");
    } else if (key == KEY_04) {
        cmd = _w1("zk");
    } else if (key == KEY_05) {
        cmd = _w1("zb");
    } else if (key == KEY_06) {
        cmd = _w1("zn");
    } else if (key == KEY_07) {
        cmd = _w1("zc");
    } else if (key == KEY_08) {
        cmd = _w1("zx");
    } else if (key == KEY_09) {
        cmd = _w1("zt");
    } else if (key == KEY_10) {
        cmd = _w1("zr");
    } else if (key == KEY_11) {
        cmd = _w1("zl");
    } else if (key == KEY_12) {
        cmd = _w1("zj");
    } else if (key == KEY_13) {
        cmd = _w1("zv");
    } else if (key == KEY_14) { // key may have caused other activities before, but required SSID now already in place
        cmd = SMART_CMD_TETHER_OFF_DEL;
    } else {
        PR05("illeg key: %02x\n", key);
    }
    if (cmd) {
        if (mysend(cmd, SMART_TARGET_HOST, SMART_TARGET_PORT, 0)) {
            PR05("cmd failed\n");
        }
    }
}

#endif

#define KEY_NONE 0xff

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
TP05
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << KEY_FAKE_1;
    gpio_config(&io_conf);
}

_u8
scan_matrix()
{
TP05
    if (gpio_get_level(KEY_FAKE_1)) {  // high if pressed
        return KEY_14;  // simulate opul
    } else {
        return KEY_NONE;
    }
}

#elif defined(ESP32_10) || \
      defined(ESP32_12)

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
init_matrix()
{
TP05
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
TP05
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
    init_matrix();
    key = scan_matrix();
#if DEBUG > 5 
    PR05("key: 0x%x [ %lu ]\n", key, esp_log_timestamp());
#if defined(KEY_OVERRIDE_V1)
    PR05("KEY_OVERRIDE: V1\n");
#endif
#if defined(KEY_OVERRIDE_V2)
    PR05("KEY_OVERRIDE: V2\n");
#endif
#endif
    init_2nd();
// -----------------------------------------------------
    if (key == KEY_NONE) {
        goto out2;
// -----------------------------------------------------
    } else if (bootCount == 1) {
PR05("-------OTA-------\n");
        if (ur_connect(OTA_SSID, 1)) {
            PR05("could not connect to %s\n", GET_SSID(OTA_SSID));
            goto out2;
        }
        esp_wifi_set_ps(WIFI_PS_NONE);              // <== (RE)CHECK THIS
        if (!esp_https_ota(&ota_config)) {
            esp_restart();
        } else {
            PR05("could not OTA\n");
            goto out1;             
        }
// -----------------------------------------------------
#if defined(ESP32_2) && !defined(KEY_OVERRIDE_V1) || defined(ESP32_12)
    } else if (key == KEY_14) {
#if DEBUG > 5
        PR05("-------1/2-------\n");
#endif
        _u32 last;
#if defined(KEY_OVERRIDE_V2)
        if (ur_connect(ROTA2G_SSID, 0)) {
            PR05("could not connect to %s\n", GET_SSID(ROTA2G_SSID));
#else
        if (ur_connect(DOOR_SSID, 0)) {
            PR05("could not connect to %s\n", GET_SSID(DOOR_SSID));
#endif
            goto out2;
        }
        esp_wifi_set_ps(WIFI_PS_NONE);              // <== (RE)CHECK THIS
        if (mysend(DOOR_CMD_ASSERT, DOOR_TARGET_HOST, DOOR_TARGET_PORT, 0)) {
            PR05("could not assert signal\n");
            goto out1;
        }
        last = wait_for_key_release();
        if (mysend(DOOR_CMD_DEASSERT, DOOR_TARGET_HOST, DOOR_TARGET_PORT, 0)) {
            PR05("could not deassert signal\n");
            goto out1;
        }
        if (last <= 4) {
            goto out1; // switch tether off only for DOOR SIGNAL exceeding a minimum time interval 
        }
        ESP_ERROR_CHECK(ur_disconnect());
#endif
    }
// -----------------------------------------------------
#if DEBUG > 5
    PR05("-------1/1 or 2/2-------\n");
#endif
#if defined(KEY_OVERRIDE_V1) && !defined(KEY_OVERRIDE_V2) || defined(ESP32_10)
    if (ur_connect(ROTA2G_SSID, 0)) {
        PR05("could not connect to %s\n", GET_SSID(ROTA2G_SSID));
#else
    if (ur_connect(SMART_SSID, 0)) {
        PR05("could not connect to %s\n", GET_SSID(SMART_SSID));
#endif
        goto out2;
    }
    esp_wifi_set_ps(WIFI_PS_NONE);              // <== (RE)CHECK THIS
    exec_cmd(key);
// -----------------------------------------------------
out1:
    ESP_ERROR_CHECK(ur_disconnect());
out2:
    wait_for_key_release(); // avoid looping through deep sleep
    beep_sync();  // wait for all queued tones to be processed 
    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(KEY_SNS_MASK, ESP_EXT1_WAKEUP_ANY_HIGH));
#if DEBUG > 5
    PR05("TP14: %lu\n", esp_log_timestamp());
#endif
    esp_deep_sleep_start();
}

