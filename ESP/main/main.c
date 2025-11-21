#include <stdio.h>
#include "Wifi.h"
#include "Data.h"
#include "Mqtt.h"
#include "Spi.h"
#define TOPIC "ecg/data"
float arr1[360];
float arr2[360];
void memset_buffer(void);
void mqtt_task(void* arg);
void app_main(void)
{
    
    memset_buffer();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI("WIFI", "ESP_WIFI_MODE_STA");
    wifi_sta_init();
    mqtt_app_start();
    spi_slave_init();
    init_spi_queue();
    subscribe_single_topic(TOPIC);
    xTaskCreate(spi_slave_task, "SPI_TASK", 4096, NULL, 10, NULL);
    xTaskCreate(mqtt_task, "MQTT_TASK", 4096, NULL, 10, NULL);
}

void memset_buffer(void)
{
    for (int i = 0; i < 360; i++)
    {
        arr1[i] = arr2[i] = 0;
    }
}
void mqtt_task(void* arg)
{
    while(1)
    {
        if (xQueueReceive(spi_data_queue, arr1, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI("MQTT_TASK", "Read from queue");
            char* json_str = create_json_one_arrays(arr1);
            publish(TOPIC, json_str);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
