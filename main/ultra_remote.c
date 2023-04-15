#include "cfg.h"
#include <string.h>
#include "sdkconfig.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/netdb.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "mlcf.h"

static const char *TAG = "TP";
static const char *REQUEST = "@beep= f:1000 c:1 t:.05 p:.25 g:-20 ^roja ^\n";
static esp_netif_t *s_example_sta_netif = NULL;
static SemaphoreHandle_t s_semph_get_ip_addrs = NULL;
static int s_retry_num = 0;

#define WIFI_CONN_MAX_RETRY 6
#define WIFI_SCAN_RSSI_THRESHOLD -127
#define WIFI_SCAN_METHOD WIFI_FAST_SCAN
#define WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#define WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN

//#undef ESP_ERROR_CHECK
//#define ESP_ERROR_CHECK(a) a
#undef ESP_LOGI
#define ESP_LOGI(a, ...)
#define NETIF_DESC_STA "ultra_remote"

static void example_handler_on_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    s_retry_num++;
    if (s_retry_num > WIFI_CONN_MAX_RETRY) {
        ESP_LOGI(TAG, "WiFi Connect failed %d times, stop reconnect.", s_retry_num);
        /* let example_wifi_sta_do_connect() return */
        if (s_semph_get_ip_addrs) {
            xSemaphoreGive(s_semph_get_ip_addrs);
        }
        return;
    }
    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED) {
        return;
    }
//    ESP_ERROR_CHECK(err);
}

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

void example_wifi_start(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//    dump_cfg("pre", &cfg);
// cfg.nvs_enable = 0x1;
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//    dump_cfg("post", &cfg);

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
    esp_netif_config.if_desc = NETIF_DESC_STA;
    esp_netif_config.route_prio = 128;
    s_example_sta_netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
    esp_wifi_set_default_wifi_sta_handlers();

#if 0
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
#else
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
#endif


    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

bool example_is_our_netif(const char *prefix, esp_netif_t *netif)
{
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

static void example_handler_on_sta_got_ip(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    s_retry_num = 0;
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    if (!example_is_our_netif(NETIF_DESC_STA, event->esp_netif)) {
        return;
    }
    ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
    if (s_semph_get_ip_addrs) {
        xSemaphoreGive(s_semph_get_ip_addrs);
    } else {
        ESP_LOGE(TAG, "- IPv4 address: " IPSTR ",", IP2STR(&event->ip_info.ip));
    }
}

static void example_handler_on_wifi_connect(void *esp_netif, esp_event_base_t event_base,
                            int32_t event_id, void *event_data)
{
}

esp_err_t example_wifi_sta_do_connect(wifi_config_t wifi_config, bool wait)
{
    if (wait) {
        s_semph_get_ip_addrs = xSemaphoreCreateBinary();
        if (s_semph_get_ip_addrs == NULL) {
            return ESP_ERR_NO_MEM;
        }
    }
    s_retry_num = 0;
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &example_handler_on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &example_handler_on_sta_got_ip, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &example_handler_on_wifi_connect, s_example_sta_netif));
    ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_err_t ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi connect failed! ret:%x", ret);
        return ret;
    }
    if (wait) {
        ESP_LOGE(TAG, "Waiting for IP(s)");
        xSemaphoreTake(s_semph_get_ip_addrs, portMAX_DELAY);
        if (s_retry_num > WIFI_CONN_MAX_RETRY) {
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

esp_err_t example_wifi_connect(void)
{
    ESP_LOGI(TAG, "Start example_connect.");
    example_wifi_start();
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
    return example_wifi_sta_do_connect(wifi_config, true);
}

esp_err_t example_wifi_sta_do_disconnect(void)
{
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &example_handler_on_wifi_disconnect));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &example_handler_on_sta_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &example_handler_on_wifi_connect));
    if (s_semph_get_ip_addrs) {
        vSemaphoreDelete(s_semph_get_ip_addrs);
    }
    return esp_wifi_disconnect();
}

void example_wifi_stop(void)
{
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
//    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(s_example_sta_netif));
    esp_netif_destroy(s_example_sta_netif);
    s_example_sta_netif = NULL;
}

void example_wifi_shutdown(void)
{
    example_wifi_sta_do_disconnect();
    example_wifi_stop();
}

void example_print_all_netif_ips(const char *prefix)
{
    // iterate over active interfaces, and print out IPs of "our" netifs
    esp_netif_t *netif = NULL;
    for (int i = 0; i < esp_netif_get_nr_of_ifs(); ++i) {
        netif = esp_netif_next(netif);
        if (example_is_our_netif(prefix, netif)) {
            ESP_LOGI(TAG, "Connected to %s", esp_netif_get_desc(netif));
#if CONFIG_LWIP_IPV4
            esp_netif_ip_info_t ip;
            ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip));

            ESP_LOGI(TAG, "- IPv4 address: " IPSTR ",", IP2STR(&ip.ip));
#endif
        }
    }
}

esp_err_t example_connect(void)
{
    if (example_wifi_connect() != ESP_OK) {
        return ESP_FAIL;
    }
    ESP_ERROR_CHECK(esp_register_shutdown_handler(&example_wifi_shutdown));
    return ESP_OK;
}

esp_err_t example_disconnect(void)
{
    example_wifi_shutdown();
    ESP_ERROR_CHECK(esp_unregister_shutdown_handler(&example_wifi_shutdown));
    return ESP_OK;
}

void app_main(void)
{
    const uint64_t ext_wakeup_pin_1_mask = 1ULL << BUTTON;
    
//    ESP_ERROR_CHECK(esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON));
//    ESP_ERROR_CHECK(rtc_gpio_pulldown_en(BUTTON));
//    ESP_ERROR_CHECK(rtc_gpio_pullup_dis(BUTTON));
    printf("Enabling EXT1 wakeup on pins GPIO%d\n", BUTTON);
    ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_1_mask, ESP_EXT1_WAKEUP_ANY_HIGH));


ESP_LOGE(TAG, "a");
    ESP_ERROR_CHECK(nvs_flash_init());
ESP_LOGE(TAG, "b");
    ESP_ERROR_CHECK(esp_netif_init());
ESP_LOGE(TAG, "c");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
ESP_LOGE(TAG, "d");

{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
#if 0
    struct in_addr *addr;
#endif
    int s, r;
    char recv_buf[128];

    while(1) {
ESP_LOGE(TAG, "1");
ESP_ERROR_CHECK(example_connect());
ESP_LOGE(TAG, "2");
        int err = getaddrinfo(TARGET_HOST, TARGET_PORT, &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
#if 0
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));
#endif
        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
ESP_LOGE(TAG, "3");
        freeaddrinfo(res);
        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
ESP_LOGE(TAG, "4");
#if 0
        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... set socket receiving timeout success");
#endif
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
#if 1
            for(int i = 0; i < r; i++) {
//                putchar(recv_buf[i]);
                if (recv_buf[i] == '\n') {
                    ESP_LOGE(TAG, "XXXXXXXXXXXXXXXXXXX");
                    goto xxx;
                }
            }
#else
            ESP_LOGE(TAG, "<%s>", recv_buf);
#endif
        } while(r > 0);
xxx:
        close(s);
ESP_ERROR_CHECK(example_disconnect());


#if 0
        for(int countdown = 100; countdown >= 0; countdown--) {
ESP_LOGE(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
#else

        printf("Entering deep sleep\n");
        esp_deep_sleep_start();


#endif


    }
}
}
