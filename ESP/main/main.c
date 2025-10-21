#include <stdio.h>
#include "Wifi.h"
#include "Data.h"
#include "Mqtt.h"
#define TOPIC "ecg/data"
float arr1[250];
float arr2[250];
void app_main(void)
{
    for (int i = 0; i < 250; i++)
    {
        arr1[i] = arr2[i] = 0.5;
    }
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI("WIFI", "ESP_WIFI_MODE_STA");
    wifi_sta_init();
    mqtt_app_start();
    subscribe_single_topic(TOPIC);
    while(1)
    {
        char* data = create_json_two_arrays(arr1, arr2);
        publish(TOPIC, data);
        for (int i = 0; i < 250; i++)
        {
            arr1[i] += 0.05;
            arr2[i] += 0.05;
        }
        vTaskDelay(500);
    }
}
