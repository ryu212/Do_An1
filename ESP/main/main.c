#include <stdio.h>
#include "time.h"
#include "Wifi.h"
#include "Data.h"
#include "Mqtt.h"
#include "Spi.h"
#include "SDCard.h"

#define TOPIC "ecg/data"
float arr1[360];
float arr2[360];
void memset_buffer(void);
void get_daily_filename(char *filename, size_t len);
void mqtt_task(void* arg);
void write_SD_card_task(void *arg);


void app_main(void)
{
    memset_buffer();
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);
        sd_card_config_t sd_config = {
        .sd_pins = {
            .mosi = MICRO_SD_MOSI,
            .miso = MICRO_SD_MISO,
            .sclk = MICRO_SD_SCLK,
            .cs   = MICRO_SD_CS
        },
        .sd_host_id = SPI2_HOST,
        .mount_point = "/sdcard",
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    ESP_ERROR_CHECK(sd_card_init(&sd_config));

    ESP_LOGI("WIFI", "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    mqtt_app_start();
    spi_slave_init();
    init_spi_queue();
    subscribe_single_topic(TOPIC);
    xTaskCreate(spi_slave_task, "SPI_TASK", 4096, NULL, 10, NULL);
    xTaskCreate(mqtt_task, "MQTT_TASK", 4096, NULL, 10, NULL);
    xTaskCreate(write_SD_card_task, "SD_TASK", 1024*6, NULL, 10, NULL);
}

void get_daily_filename(char *filename, size_t len)
{
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    strftime(filename, len, "ecg_%Y_%m_%d.bin", &timeinfo);
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
        if (xQueueReceive(mqtt_data_queue, arr1, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI("MQTT_TASK", "Read from queue");
            char* json_str = create_json_one_arrays(arr1);
            publish(TOPIC, json_str);
            free(json_str);             
        }
        vTaskDelay(pdMS_TO_TICKS(10));

    }
}

void write_SD_card_task(void *arg)
{
    float sd_batch[10][360];
    int batch_count = 0;
    char filename[64];

    while (1)
    {
        if (xQueueReceive(sd_card_data_queue, sd_batch[batch_count], portMAX_DELAY) == pdTRUE) {
            ESP_LOGI("SD_TASK", "Read block %d from queue", batch_count + 1);
            batch_count++;
        }

        if (batch_count >= 10) {
            get_daily_filename(filename, sizeof(filename));

            esp_err_t ret = sd_card_append(filename,
                                           (const uint8_t *)sd_batch,
                                           sizeof(sd_batch));
            if (ret == ESP_OK) {
                ESP_LOGI("SD_TASK", "Write 10 blocks to SD card success");
            } else {
                ESP_LOGE("SD_TASK", "Write 10 blocks to SD card failed");
            }
            batch_count = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}