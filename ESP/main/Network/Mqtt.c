#include "Mqtt.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include <time.h>
static const char *TAG = "MQTT_CLIENT";
esp_mqtt_client_handle_t mqtt_client = NULL;
QueueHandle_t mqtt_message_queue = NULL;  // Queue to store incoming MQTT messages


extern const uint8_t hivemq_cert_pem_start[] asm("_binary_CA_pem_start");
extern const uint8_t hivemq_cert_pem_end[]   asm("_binary_CA_pem_end");

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
        mqtt_message_t message; // Create a message struct to hold the topic and payload
        memset(&message, 0, sizeof(message)); // clear all fields in message struct
        /*
        check len topic and payload before coppy to avoid buffer overflow 
        */
        if (event->topic_len < sizeof(message.topic) && event->data_len < sizeof(message.payload)) {
            memcpy(message.topic, event->topic, event->topic_len);
            message.topic[event->topic_len] = '\0';
            memcpy(message.payload, event->data, event->data_len);
            message.payload[event->data_len] = '\0';
            /*
            Send the message to the queue for processing in the mqtt_message_queue_task
            because app can send many massage at the same time
            */
            if(xQueueSend(mqtt_message_queue, &message, portMAX_DELAY) != pdPASS) {
                ESP_LOGE(TAG, "Failed to send MQTT message to queue");
            }
            else{
                ESP_LOGI(TAG, "MQTT message sent to queue successfully");
            }
        } else {
            ESP_LOGW(TAG, "Received topic or payload is too long, skipping");
        }
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
     // Create a queue to hold up to 10 MQTT messages
    mqtt_message_queue = xQueueCreate(10, sizeof(mqtt_message_t));
    if (mqtt_message_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create MQTT message queue");
        return;
    }
    esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = MQTT_BROKER_URI,
    .credentials.username = MQTT_USERNAME,
    .credentials.authentication.password = MQTT_PASSWORD,
    .broker.verification.certificate = (const char *)hivemq_cert_pem_start,
    .broker.verification.certificate_len = hivemq_cert_pem_end - hivemq_cert_pem_start,
    .broker.verification.skip_cert_common_name_check = false,
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