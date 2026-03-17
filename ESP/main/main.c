#include "Controller.h"

void app_main(void)
{
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
    wifi_init_sta(); //initialize wifi
    mqtt_app_start(); // initialize mqtt
    spi_slave_init(); // initial spi slave to receive data from stm32
    init_spi_queue(); // initialize mqtt_queue and sd_queue to push received data from spi to avoid blockage 
    xTaskCreate(spi_slave_task, "SPI_TASK", 4096, NULL, 10, NULL);
    xTaskCreate(mqtt_task, "MQTT_TASK", 4096, NULL, 10, NULL);
    xTaskCreate(write_SD_card_task, "SD_TASK", 1024*6, NULL, 10, NULL);
}
