#ifndef MQTT_H
#define MQTT_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"          // thay cho esp_event_loop.h (deprecated)

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"

#ifdef __cplusplus
extern "C" {
#endif
// extern const uint8_t hivemq_cert_pem_start[] asm("_binary_hivemq_cert_pem_start");
// extern const uint8_t hivemq_cert_pem_end[]   asm("_binary_hivemq_cert_pem_end");
// KHÔNG define TAG ở header. Đặt TAG trong .c để tránh xung đột.
extern esp_mqtt_client_handle_t mqtt_client;

// Prototype đúng chuẩn ESP-IDF 5.x cho event handler
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

void mqtt_app_start(void);
void publish(const char *topic, const char *data);
void subscribe_single_topic(const char *topic);
void unsubscribe_single_topic(const char *topic);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_H */