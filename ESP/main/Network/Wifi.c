#include "Wifi.h"


#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *tag = "wifi_sta";

#define MAX_TRY 10
#define BASE_DELAY_MS 250
#define MAX_DELAY_MS  5000

static EventGroupHandle_t wifi_event_group;
static esp_timer_handle_t retry_timer;
static uint8_t retry_count;

static esp_err_t nvs_init_safe(void)
{
    esp_err_t ret = nvs_flash_init();

    /* nvs can become incompatible after sdk updates or if flash pages are exhausted
       erasing and re-initializing is the recommended recovery path for those cases */
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    return ret;
}

static void retry_timer_cb(void *arg)
{
    (void)arg;

    /* reconnect is performed from a timer task context to avoid tight reconnect loops
       this improves stability on weak wifi signals and reduces blocking inside event callbacks */
    ESP_LOGI(tag, "retrying wifi connect");
    esp_wifi_connect();
}

static uint32_t compute_backoff_delay_ms(uint8_t attempt)
{
    /* exponential backoff helps avoid hammering the access point
       the delay is capped to keep recovery time reasonable */
    uint32_t exp = (attempt < 10) ? attempt : 10;
    uint32_t delay_ms = BASE_DELAY_MS * (1u << exp);
    if (delay_ms > MAX_DELAY_MS) delay_ms = MAX_DELAY_MS;
    return delay_ms;
}

static void schedule_retry(void)
{
    uint32_t delay_ms = compute_backoff_delay_ms(retry_count);

    /* retry_count is incremented before scheduling so logs reflect the upcoming attempt */
    retry_count++;

    ESP_LOGI(tag, "scheduling reconnect attempt %u/%u after %u ms",
             (unsigned)retry_count, (unsigned)MAX_TRY, (unsigned)delay_ms);

    if (retry_timer) {
        esp_timer_stop(retry_timer);
        esp_timer_start_once(retry_timer, (uint64_t)delay_ms * 1000ull);
    } else {
        /* this fallback should rarely happen, but it prevents a silent failure path */
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        esp_wifi_connect();
    }
}

static void wifi_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        /* station mode has started, trigger the first connection attempt */
        retry_count = 0;
        ESP_LOGI(tag, "wifi started, connecting");
        esp_wifi_connect();
        return;
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *disc = (wifi_event_sta_disconnected_t *)event_data;
        int reason = disc ? disc->reason : -1;

        /* do not treat most disconnect reasons as fatal because lab networks can be noisy
           auth failures are a strong signal that credentials are wrong, so we stop early */
        ESP_LOGW(tag, "disconnected, reason=%d, retry=%u/%u",
                 reason, (unsigned)retry_count, (unsigned)MAX_TRY);

        if (disc && disc->reason == WIFI_REASON_AUTH_FAIL) {
            ESP_LOGE(tag, "auth failed, check ssid and password");
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
            return;
        }

        if (retry_count < MAX_TRY) {
            schedule_retry();
        } else {
            ESP_LOGE(tag, "failed to connect after %u retries", (unsigned)MAX_TRY);
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
        return;
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        /* once an ip is acquired, the network path is ready for mqtt tls connections */
        ESP_LOGI(tag, "got ip: " IPSTR, IP2STR(&event->ip_info.ip));

        retry_count = 0;

        /* stop any pending retry timer to avoid reconnect attempts while connected */
        if (retry_timer) {
            esp_timer_stop(retry_timer);
        }

        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        return;
    }
}

void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();
    configASSERT(wifi_event_group);

    ESP_ERROR_CHECK(nvs_init_safe());
    ESP_ERROR_CHECK(esp_netif_init());

    /* the default event loop may already exist in some projects
       this guard avoids failing when wifi is re-initialized */
    esp_err_t er = esp_event_loop_create_default();
    if (er != ESP_OK && er != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(er);
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* register for both wifi and ip events using the same handler
       this matches the typical pattern in esp-idf station examples */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_ip_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_ip_event_handler, NULL, NULL));

    esp_timer_create_args_t targs = {
        .callback = &retry_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wifi_retry"
    };
    ESP_ERROR_CHECK(esp_timer_create(&targs, &retry_timer));

    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config));

    /* ssid and password are provided by your KEY_INFO.h constants */
    strncpy((char *)wifi_config.sta.ssid, SSID_WIFI, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, PASS_WIFI, sizeof(wifi_config.sta.password));

    /* this threshold improves security by refusing open networks
       if your lab router is wpa2/wpa3 mixed and you see connection issues,
       switch this to WIFI_AUTH_WPA_WPA2_PSK or remove the threshold */
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    /* disabling power save reduces latency and improves mqtt realtime reliability */
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    retry_count = 0;

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(tag, "wifi init done, waiting for connection");

    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(tag, "connected to ap");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(tag, "failed to connect to ap");
    } else {
        ESP_LOGE(tag, "unexpected event group bits: 0x%02x", (unsigned)bits);
    }
}

bool wifi_is_connected(void)
{
    if (wifi_event_group == NULL) {
        return false;
    }

    EventBits_t bits = xEventGroupGetBits(wifi_event_group);
    return (bits & WIFI_CONNECTED_BIT) != 0;
}
