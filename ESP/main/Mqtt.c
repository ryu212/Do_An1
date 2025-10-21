#include "Mqtt.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include <time.h>
static const char *TAG = "MQTT_CLIENT";
esp_mqtt_client_handle_t mqtt_client = NULL;

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "Subscribe qos0, msg_id=%d", msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;

    default:
        ESP_LOGI(TAG, "Unhandled event id: %d", event->event_id);
        break;
    }
}

void mqtt_app_start(void)
{
    
    //obtain_time();
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://172.17.0.1:1883",
        /*.credentials.username = "Ldz123",
        .credentials.authentication.password = "Longdeptrai123",
        .broker.verification.certificate = (const char *)hivemq_cert_pem_start,
        .broker.verification.certificate_len = hivemq_cert_pem_end - hivemq_cert_pem_start,
        .broker.verification.skip_cert_common_name_check = false,*/
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void publish(const char *topic, const char *data)
{
    if (mqtt_client) esp_mqtt_client_publish(mqtt_client, topic, data, strlen(data), 1, 1);
}

void subscribe_single_topic(const char *topic)
{
    if (mqtt_client) esp_mqtt_client_subscribe(mqtt_client, topic, 1);
}

void unsubscribe_single_topic(const char *topic)
{
    if (mqtt_client) esp_mqtt_client_unsubscribe(mqtt_client, topic);
}