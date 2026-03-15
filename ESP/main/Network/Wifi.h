#ifndef WIFI_H
#define WIFI_H

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <stdbool.h>
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "secure_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

void wifi_init_sta(void);
bool wifi_is_connected(void);
#ifdef __cplusplus
}
#endif

#endif


