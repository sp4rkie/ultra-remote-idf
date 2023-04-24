#if defined(ESP_0)
#define HOST "esp_0"
#elif defined(ESP_1)
#define HOST "esp_1"
#elif defined(ESP_2)
#define HOST "esp_2"

#elif defined(ESP32_0)
#define HOST "esp32_0"
#elif defined(ESP32_1)
#define HOST "esp32_1"
#elif defined(ESP32_2)
#define HOST "esp32_2"
#elif defined(ESP32_3)
#define HOST "esp32_3"
#elif defined(ESP32_4)
#define HOST "esp32_4"
#elif defined(ESP32_5)
#define HOST "esp32_5"
#elif defined(ESP32_6)
#define HOST "esp32_6"
#elif defined(ESP32_7)
#define HOST "esp32_7"
#elif defined(ESP32_8)
#define HOST "esp32_8"
#elif defined(ESP32_9)
#define HOST "esp32_9"

#elif defined(ESP32_10)
#define HOST "esp32_10"
#elif defined(ESP32_11)
#define HOST "esp32_11"
#elif defined(ESP32_12)
#define HOST "esp32_12"
#elif defined(ESP32_13)
#define HOST "esp32_13"
#elif defined(ESP32_14)
#define HOST "esp32_14"
#elif defined(ESP32_15)
#define HOST "esp32_15"
#elif defined(ESP32_16)
#define HOST "esp32_16"
#elif defined(ESP32_17)
#define HOST "esp32_17"
#elif defined(ESP32_18)
#define HOST "esp32_18"
#elif defined(ESP32_19)
#define HOST "esp32_19"
#elif defined(ESP32_20)
#define HOST "esp32_20"

#else 

#error HOST "undefined"

#endif

/* ---vvv--- debug ---vvv--- */
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
//TP05
    touch_pad_t touchPin = esp_sleep_get_touchpad_wakeup_status();

    switch (touchPin) {
        case 0  : PR05("Touch detected on GPIO 4\n"); break;
        case 1  : PR05("Touch detected on GPIO 0\n"); break;
        case 2  : PR05("Touch detected on GPIO 2\n"); break;
        case 3  : PR05("Touch detected on GPIO 15\n"); break;
        case 4  : PR05("Touch detected on GPIO 13\n"); break;
        case 5  : PR05("Touch detected on GPIO 12\n"); break;
        case 6  : PR05("Touch detected on GPIO 14\n"); break;
        case 7  : PR05("Touch detected on GPIO 27\n"); break;
        case 8  : PR05("Touch detected on GPIO 33\n"); break;
        case 9  : PR05("Touch detected on GPIO 32\n"); break;
        default : PR05("wakeup not by touchpad\n"); break;
    }
}

void
print_wakeup_gpio_wakeup()
{
//TP05
#ifdef SOC_GPIO_SUPPORT_DEEPSLEEP_WAKEUP    // only esp32c3 defines this?!?
    _u64 GpioPin = esp_sleep_get_gpio_wakeup_status();

    PR05("wakeup by gpio [ 0x%016llx ], PIN: %u\n", GpioPin, ffs(GpioPin) - 1);
#else

    /*
     * there solely exists exactly the one defined by 
     *     esp_sleep_enable_ext0_wakeup(PIN, 0);
     */
    PR05("wakeup by ext0 PIN X, check esp_sleep_enable_ext0_wakeup()\n");
#endif
}

void 
print_wakeup_ext1_wakeup()
{
//TP05
    _u64 GpioPin = esp_sleep_get_ext1_wakeup_status();

    PR05("wakeup by ext1 [ 0x%016llx ], PIN: %u\n", GpioPin, ffs(GpioPin) - 1);
}

void 
print_wakeup_reason()
{
//TP05
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            PR05("wakeup caused by external signal using RTC_IO (EXT0)\n"); 
            print_wakeup_gpio_wakeup();
            break;
        case ESP_SLEEP_WAKEUP_EXT1: 
            PR05("wakeup caused by external signal using RTC_CNTL (EXT1)\n"); 
            print_wakeup_ext1_wakeup();
            break;
        case ESP_SLEEP_WAKEUP_TIMER: 
            PR05("wakeup caused by timer\n"); 
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: 
            PR05("wakeup caused by touchpad\n"); 
            print_wakeup_touchpad();
            break;
        case ESP_SLEEP_WAKEUP_ULP: 
            PR05("wakeup caused by ULP program\n"); 
            break;
        default: 
            PR05("wakeup was not caused by deep sleep (reason == %u)\n", wakeup_reason); 
            break;
    }
}

#ifdef ESP_IDF_VERSION_MAJOR // IDF 4+
#if CONFIG_IDF_TARGET_ESP32 // ESP32/PICO-D4
#include "esp32/rom/rtc.h"                      // <== OURs!
#elif CONFIG_IDF_TARGET_ESP32S2
//#include "esp32s2/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32C3
//#include "esp32c3/rom/rtc.h"
#elif CONFIG_IDF_TARGET_ESP32S3
//#include "esp32s3/rom/rtc.h"
#else
#error Target CONFIG_IDF_TARGET is not supported
#endif
#else // ESP32 Before IDF 4.0
//#include "rom/rtc.h"
#endif

void 
print_reset_reason(_u32 cpu)
{
//TP05
    _i32 reason = rtc_get_reset_reason(cpu);

    PR05("CPU%d reset reason: ", cpu);
    switch (reason) {
        case 1: 
            PR05("Vbat power on reset\n");
            break;
        case 3: 
            PR05("Software reset digital core\n");
            break;
        case 4: 
            PR05("Legacy watch dog reset digital core\n");
            break;
        case 5: 
            PR05("Deep Sleep reset digital core\n");
            break;
        case 6: 
            PR05("Reset by SLC module, reset digital core\n");
            break;
        case 7: 
            PR05("Timer Group0 Watch dog reset digital core\n");
            break;
        case 8: 
            PR05("Timer Group1 Watch dog reset digital core\n");
            break;
        case 9: 
            PR05("RTC Watch dog Reset digital core\n");
            break;
        case 10: 
            PR05("Instrusion tested to reset CPU\n");
            break;
        case 11: 
            PR05("Time Group reset CPU\n");
            break;
        case 12: 
            PR05("Software reset CPU\n");
            break;
        case 13: 
            PR05("RTC Watch dog Reset CPU\n");
            break;
        case 14: 
            PR05("for APP CPU, reseted by PRO CPU\n");
            break;
        case 15: 
            PR05("Reset when the vdd voltage is not stable\n");
            break;
        case 16: 
            PR05("RTC Watch dog reset digital core and rtc module\n");
            break;
        default: 
            PR05("NO_MEAN\n");
            break;
    }
}

/*
0  SYSTEM_EVENT_WIFI_READY               < ESP32 WiFi ready
1  SYSTEM_EVENT_SCAN_DONE                < ESP32 finish scanning AP
2  SYSTEM_EVENT_STA_START                < ESP32 station start
3  SYSTEM_EVENT_STA_STOP                 < ESP32 station stop
4  SYSTEM_EVENT_STA_CONNECTED            < ESP32 station connected to AP
5  SYSTEM_EVENT_STA_DISCONNECTED         < ESP32 station disconnected from AP
6  SYSTEM_EVENT_STA_AUTHMODE_CHANGE      < the auth mode of AP connected by ESP32 station changed
7  SYSTEM_EVENT_STA_GOT_IP               < ESP32 station got IP from connected AP
8  SYSTEM_EVENT_STA_LOST_IP              < ESP32 station lost IP and the IP is reset to 0
9  SYSTEM_EVENT_STA_WPS_ER_SUCCESS       < ESP32 station wps succeeds in enrollee mode
10 SYSTEM_EVENT_STA_WPS_ER_FAILED        < ESP32 station wps fails in enrollee mode
11 SYSTEM_EVENT_STA_WPS_ER_TIMEOUT       < ESP32 station wps timeout in enrollee mode
12 SYSTEM_EVENT_STA_WPS_ER_PIN           < ESP32 station wps pin code in enrollee mode
13 SYSTEM_EVENT_AP_START                 < ESP32 soft-AP start
14 SYSTEM_EVENT_AP_STOP                  < ESP32 soft-AP stop
15 SYSTEM_EVENT_AP_STACONNECTED          < a station connected to ESP32 soft-AP
16 SYSTEM_EVENT_AP_STADISCONNECTED       < a station disconnected from ESP32 soft-AP
17 SYSTEM_EVENT_AP_STAIPASSIGNED         < ESP32 soft-AP assign an IP to a connected station
18 SYSTEM_EVENT_AP_PROBEREQRECVED        < Receive probe request packet in soft-AP interface
19 SYSTEM_EVENT_GOT_IP6                  < ESP32 station or ap or ethernet interface v6IP addr is preferred
20 SYSTEM_EVENT_ETH_START                < ESP32 ethernet start
21 SYSTEM_EVENT_ETH_STOP                 < ESP32 ethernet stop
22 SYSTEM_EVENT_ETH_CONNECTED            < ESP32 ethernet phy link up
23 SYSTEM_EVENT_ETH_DISCONNECTED         < ESP32 ethernet phy link down
24 SYSTEM_EVENT_ETH_GOT_IP               < ESP32 ethernet got IP from connected AP
25 SYSTEM_EVENT_MAX
*/

#if 0
void
WiFiEvent(WiFiEvent_t event) {
TP05
//  PR05("[WiFi-event] event: %u\n", event);
  PR05("\nEV: %u ", (_u32)esp_timer_get_time() / portTICK_PERIOD_MS);

  switch (event) {
    case SYSTEM_EVENT_WIFI_READY: 
      println("WiFi interface ready");
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      println("Completed scan for access points");
      break;
    case SYSTEM_EVENT_STA_START:
      println("WiFi client started");
      break;
    case SYSTEM_EVENT_STA_STOP:
      println("WiFi clients stopped");
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      println("Connected to access point");
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      println("Disconnected from WiFi access point");
      WIFI_DIAG(0);
      break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
      println("Authentication mode of access point has changed");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      printf("Obtained IP address");
      WIFI_DIAG(1);
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      println("Lost IP address and IP address is reset to 0");
      break;
    case SYSTEM_EVENT_STA_BSS_RSSI_LOW:
      println("WiFi SYSTEM_EVENT_STA_BSS_RSSI_LOW");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      println("WiFi Protected Setup (WPS): failed in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      println("WiFi Protected Setup (WPS): timeout in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      println("WiFi Protected Setup (WPS): pin code in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PBC_OVERLAP:
      println("WiFi SYSTEM_EVENT_STA_WPS_ER_PBC_OVERLAP");
      break;
    case SYSTEM_EVENT_AP_START:
      println("WiFi access point started");
      break;
    case SYSTEM_EVENT_AP_STOP:
      println("WiFi access point  stopped");
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      println("Client connected");
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      println("Client disconnected");
      break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
      println("Assigned IP address to client");
      break;
    case SYSTEM_EVENT_AP_PROBEREQRECVED:
      println("Received probe request");
      break;
    case SYSTEM_EVENT_ACTION_TX_STATUS:
      println("WiFi SYSTEM_EVENT_ACTION_TX_STATUS");
      break;
    case SYSTEM_EVENT_ROC_DONE:
      println("WiFi SYSTEM_EVENT_ROC_DONE");
      break;
    case SYSTEM_EVENT_STA_BEACON_TIMEOUT:
      println("WiFi SYSTEM_EVENT_STA_BEACON_TIMEOUT");
      break;
    case SYSTEM_EVENT_FTM_REPORT:
      println("WiFi SYSTEM_EVENT_FTM_REPORT");
      break;
    case SYSTEM_EVENT_GOT_IP6:
      println("IPv6 is preferred");
      break;
    case SYSTEM_EVENT_ETH_START:
      println("Ethernet started");
      break;
    case SYSTEM_EVENT_ETH_STOP:
      println("Ethernet stopped");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      println("Ethernet connected");
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      println("Ethernet disconnected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      println("Obtained IP address");
      break;
    case SYSTEM_EVENT_ETH_LOST_IP:
      println("WiFi SYSTEM_EVENT_ETH_LOST_IP");
      break;
    default:
      printf("============= NO CASE [ %u ] ==============\n", event);
      break;
  }
}
#endif
/* ---^^^--- debug ---^^^--- */

/* ---vvv--- acoustic feedback ---vvv--- */
#define     BEEP_2540   2540
#define     BEEP_1300   1300
#define     BEEP_1000   1000
#define     BEEP_OK     BEEP_2540
#define     BEEP_ERR    BEEP_1000
#define     BEEP_INFO   BEEP_1300

#ifdef BUZZER

#define SINGLE_PULSE_WIDTH 300
#define MULTI_PULSE_WIDTH 70
#define MULTI_PULSE_PAUSE 250

_u32 tone_duration;
_u32 tone_last_syncpoint;

void
beep_sync() 
{
TP05
    _i32 tmp = tone_duration - ((_u32)esp_timer_get_time() / portTICK_PERIOD_MS - tone_last_syncpoint);

    if (tmp > 0) {
if (DEBUG > 5) PR05("syncing tone(s) for %dms\n", tmp);
        vTaskDelay(tmp / portTICK_PERIOD_MS);
    }
    tone_last_syncpoint = (_u32)esp_timer_get_time() / portTICK_PERIOD_MS;
    tone_duration = 0;
}

void
beep(_u32 frequ, _u32 cnt)
{
TP05
    beep_sync();
    if (cnt > 1) {
        tone_duration += cnt * MULTI_PULSE_WIDTH + (cnt - 1) * MULTI_PULSE_PAUSE;
        --cnt;
        tone(BUZZER, frequ, MULTI_PULSE_WIDTH);
        while (cnt--) {
            tone(BUZZER, 0, MULTI_PULSE_PAUSE); // effective silent
            tone(BUZZER, frequ, MULTI_PULSE_WIDTH);
        }
    } else {
        tone_duration += SINGLE_PULSE_WIDTH;
        tone(BUZZER, frequ, SINGLE_PULSE_WIDTH);
    }
}
#else
void beep_sync() {}
void beep(_u32 frequ, _u32 cnt) {}
#endif
/* ---^^^--- acoustic feedback ---^^^--- */

/* ---vvv--- WiFi section ---vvv--- */
#define NA_SSID 0
#define OTA_SSID 1
#define DOOR_SSID 2
#define SMART_SSID 3
#define ROTA2G_SSID 4
#define ROTA5G_SSID 5

static _i32 s_retry_num = 0;
static esp_netif_t *s_ur_sta_netif = 0;
static SemaphoreHandle_t s_semph_get_ip_addrs = 0;

_i8p accpts[] = {
    WIFI0_SSID, WIFI0_PASSWORD,
    WIFI1_SSID, WIFI1_PASSWORD,
    WIFI2_SSID, WIFI2_PASSWORD,
    WIFI3_SSID, WIFI3_PASSWORD,
    WIFI4_SSID, WIFI4_PASSWORD,
    WIFI5_SSID, WIFI5_PASSWORD,
};

#define WIFI_CONN_MAX_RETRY 6
#define WIFI_SCAN_RSSI_THRESHOLD -127
#define WIFI_SCAN_METHOD WIFI_FAST_SCAN
#define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#define WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL

#define NETIF_DESC_STA "ur"

void
dump_cfg(const _i8 *str, wifi_init_config_t *cfg)
{
    PR05("%s", str);
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
    PR05_("\n");
}

bool
ur_is_our_netif(const _i8 *prefix, esp_netif_t *netif)
{
TP05
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

static void
ur_handler_on_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
TP05
    s_retry_num++;
    if (s_retry_num > WIFI_CONN_MAX_RETRY) {
        PR05("WiFi Connect failed %d times, stop reconnect.\n", s_retry_num);
        if (s_semph_get_ip_addrs) {
            xSemaphoreGive(s_semph_get_ip_addrs);
        }
        return;
    }
    PR05("Wi-Fi disconnected, trying to reconnect...\n");
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED) {
        return;
    }
    ESP_ERROR_CHECK(err);
}

static void
ur_handler_on_sta_got_ip(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
TP05
    s_retry_num = 0;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    if (!ur_is_our_netif(NETIF_DESC_STA, event->esp_netif)) {
        return;
    }
    if (s_semph_get_ip_addrs) {
        xSemaphoreGive(s_semph_get_ip_addrs);
    } else {
        PR05("out of the blue IPv4 address: " IPSTR "\n", IP2STR(&event->ip_info.ip));
    }
}

static void
ur_handler_on_wifi_connect(void *esp_netif, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
TP05
}

void
ur_wifi_start(void)
{
TP05
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//dump_cfg("pre", &cfg);
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    esp_netif_config.if_desc = NETIF_DESC_STA;
    esp_netif_config.route_prio = 128;
    s_ur_sta_netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
    esp_wifi_set_default_wifi_sta_handlers();
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

esp_err_t
ur_wifi_sta_do_connect(wifi_config_t wifi_config, bool wait)
{
TP05
    if (wait) {
        s_semph_get_ip_addrs = xSemaphoreCreateBinary();
        if (!s_semph_get_ip_addrs) {
            return ESP_ERR_NO_MEM;
        }
    }
    s_retry_num = 0;
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &ur_handler_on_wifi_disconnect, 0));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ur_handler_on_sta_got_ip, 0));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &ur_handler_on_wifi_connect, s_ur_sta_netif));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_err_t ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        PR05("WiFi connect failed! ret:%x\n", ret);
        return ret;
    }
    if (wait) {
        xSemaphoreTake(s_semph_get_ip_addrs, portMAX_DELAY); // effective no timeout
        if (s_retry_num > WIFI_CONN_MAX_RETRY) {
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t
ur_wifi_connect(_u8 ssid)
{
TP05
    ur_wifi_start();
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI4_SSID,
            .password = WIFI4_PASSWORD,
            .scan_method = WIFI_SCAN_METHOD,
            .sort_method = WIFI_CONNECT_AP_SORT_METHOD,
            .threshold.rssi = WIFI_SCAN_RSSI_THRESHOLD,
            .threshold.authmode = WIFI_SCAN_AUTH_MODE_THRESHOLD,
        },
    };
    strcpy((_i8p)wifi_config.sta.ssid, accpts[(ssid << 1) + 0]);
    strcpy((_i8p)wifi_config.sta.password, accpts[(ssid << 1) + 1]);
    return ur_wifi_sta_do_connect(wifi_config, true);
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
    ur_wifi_sta_do_disconnect();
    ur_wifi_stop();
}

void
ur_print_all_netif_ips(const _i8 *prefix)
{
TP05
    // iterate over active interfaces, and print out IPs of "our" netifs
    esp_netif_t *netif = 0;
    for (_i32 i = 0; i < esp_netif_get_nr_of_ifs(); ++i) {
        netif = esp_netif_next(netif);
        if (ur_is_our_netif(prefix, netif)) {
            PR05("Connected to %s\n", esp_netif_get_desc(netif));
            esp_netif_ip_info_t ip;
            ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip));
            PR05("- IPv4 address: " IPSTR ",", IP2STR(&ip.ip));
        }
    }
}

esp_err_t
ur_connect(_u8 ssid)
{
TP05
    if (ur_wifi_connect(ssid) != ESP_OK) {
        return ESP_FAIL;
    }
    ESP_ERROR_CHECK(esp_register_shutdown_handler(&ur_wifi_shutdown));
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
/* ---^^^--- WiFi section ---^^^--- */

/* ---vvv--- OTA section ---vvv--- */
esp_err_t
_http_event_handler(esp_http_client_event_t *evt)
{
//TP05
    static _u32 sum;
    static _u32 summed;
    static _u32 last;
    _u32 now = esp_log_timestamp();

    switch (evt->event_id) {
      case HTTP_EVENT_ERROR:
        PR05("HTTP_EVENT_ERROR\n");
        break;
      case HTTP_EVENT_ON_CONNECTED:
//PR05("HTTP_EVENT_ON_CONNECTED\n");
        break;
      case HTTP_EVENT_HEADER_SENT:
//PR05("HTTP_EVENT_HEADER_SENT\n");
        break;
      case HTTP_EVENT_ON_HEADER:
//PR05("HTTP_EVENT_ON_HEADER, key=%s, value=%s\n", evt->header_key, evt->header_value);
        if (!strcmp(evt->header_key, "Content-Length")) {
            PR05("file size to load: %s\n", evt->header_value);
            sum = atoi(evt->header_value);
            summed = 0;
            last = 0;
        }
        break;
      case HTTP_EVENT_ON_DATA:
        if (sum) {
            summed += evt->data_len;
            if (now - last > 1000) {
                PR05("\r %d%%", summed * 100 / sum);
                fflush(stdout);
                last = now;
            }
        }
        break;
      case HTTP_EVENT_ON_FINISH:
        PR05("HTTP_EVENT_ON_FINISH\n");
        break;
      case HTTP_EVENT_DISCONNECTED:
//PR05("HTTP_EVENT_DISCONNECTED\n");
        if (sum) {
            PR05("\r %d%%", summed * 100 / sum);
            PR05("\n");
            sum = 0; // avoid double PR05 on secnd HTTP_EVENT_DISCONNECTED
        }
        break;
      case HTTP_EVENT_REDIRECT:
        PR05("HTTP_EVENT_REDIRECT\n");
        break;
    }
    return ESP_OK;
}

esp_http_client_config_t config = {
    .url = "http://" OTA_MACHINE ":8070/" OTA_IMAGE,
    .event_handler = _http_event_handler,
    .keep_alive_enable = true,
};
esp_https_ota_config_t ota_config = {
    .http_config = &config,
};
/* ---^^^--- OTA section ---^^^--- */

#ifdef TARGET_PORT
/* ---vvv--- cmd to status ---vvv--- */

#include <regex.h>

/*
 * consider
 *    bin/pq.awklib [ STATUS_MATCH ]
 *    bin/p         [ server_status ]
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
_i8p STATUS_MATCH =
                                        // <== in arduino-esp32 JUST ONE \ COMPARED TO l6.awklib?!?!?!?
                                        // <== in esp-idf NONE \ at all COMPARED TO l6.awklib?!?!?!?
                "^#\\[([^]]+)\\]" \
                 "#\\[([01])\\]" \
                 "#\\[([01])\\]" \
                 "#\\[([a-z0-9]+)\\]" \
                 "#\\[([^()]+)\\]" \
                 "#\\[([01])\\]$";
regex_t _regex;
regmatch_t _pmatch[7];  // nr of parenthesized subexprs in STATUS_MATCH + 1
#define STAT_CMD 1     // parenthesized subexprs indices
#define STAT_REPEAT 2
#define STAT_STEALTH 3
#define STAT_PLAY_MODE 4
#define STAT_MISC 5
#define STAT_STAT 6

_i8 _buf[128];
_i8p _err[] = {
    "-----",   // sample text

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

_i32
mysend(_i8p cmd, _i8p host, _i8p port, _i8p *statmsg)
{
TP05
if (DEBUG > 5) PR05("cmd: %lu %s -> %s@%s\n", esp_log_timestamp(), cmd, host, port);
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    _i8 cr_buf[128];
    _i32 s, r, err, stat;

    stat = 0;
    err = getaddrinfo(host, port, &hints, &res);
    if (err || !res) {
        PR05("DNS lookup failed err=%d res=%p\n", err, res);
        stat = 2;
        goto out1;
    }
    s = socket(res->ai_family, res->ai_socktype, 0);
    if (s < 0) {
        PR05("... Failed to allocate socket.\n");
        freeaddrinfo(res);
        stat = 2;
        goto out2;
    }
    if (connect(s, res->ai_addr, res->ai_addrlen) != 0) {
        PR05("... socket connect failed errno=%d\n", errno);
        close(s);
        freeaddrinfo(res);
        stat = 2;
        goto out2;
    }
    freeaddrinfo(res);
    snprintf(cr_buf, _SZ(cr_buf), "%s\n", cmd);
    if (write(s, cr_buf, strlen(cr_buf)) < 0) {
        PR05("... socket send failed\n");
        close(s);
        stat = 3;
        goto out2;
    }
    do {
        r = read(s, cr_buf, sizeof(cr_buf) - 1);
        if (cr_buf[r - 1] == '\n') {
            cr_buf[r - 1] = 0;
if (DEBUG > 5) PR05("stat: %lu %s\n", esp_log_timestamp(), cr_buf);
            if (err = regexec(&_regex, cr_buf, _NE(_pmatch), _pmatch, 0)) {
                // no match
                regerror(err, &_regex, _buf, _SZ(_buf));
if (DEBUG > 1) PR05("%s\n", _buf);
                stat = 4;
            } else if (strncmp("0", cr_buf + _pmatch[STAT_STAT].rm_so, 1)) {
if (DEBUG > 1) PR05("status failed\n");
                stat = 5;
            } else {
                // valid res
                *_buf = 0;
                strncat(_buf, cr_buf + _pmatch[STAT_MISC].rm_so, _pmatch[STAT_MISC].rm_eo - _pmatch[STAT_MISC].rm_so);
                if (statmsg) *statmsg = _buf;                                      
                stat = 0;
            }
            goto out2;
        }
    } while (r > 0);
out2:
    close(s);
out1:
    if (stat) {
        if (statmsg) *statmsg = _err[stat];
        beep(BEEP_ERR, 3);
    } else {
        beep(BEEP_OK, 1);
    }
    return stat;
}
/* ---^^^--- cmd to status ---^^^--- */
#endif

RTC_DATA_ATTR _u32 bootCount = 0;

_i32
init_1st()
{
//TP05
    _i32 err;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ++bootCount;
if (DEBUG > 5) PR05("bootCount: %d\n", bootCount);

if (DEBUG > 5) print_reset_reason(0);
if (DEBUG > 5) print_reset_reason(1);
if (DEBUG > 5) print_wakeup_reason();
//if (DEBUG > 5) WiFi.onEvent(WiFiEvent);

#ifdef TARGET_PORT
    if (err = regcomp(&_regex, STATUS_MATCH, REG_EXTENDED)) {
        regerror(err, &_regex, _buf, _SZ(_buf));
        PR05("%s\n", _buf);
    }
#endif
    return 0;
}

