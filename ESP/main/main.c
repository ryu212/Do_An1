#include <stdio.h>
#include "Wifi.h"
#include "Data.h"
#include "Mqtt.h"
#include "SPI.h"

#define TAG   "error"
#define TOPIC "ecg/data"

static float arr1[ECG_SAMPLES_PER_FRAME];
static float arr2[ECG_SAMPLES_PER_FRAME];

void app_main(void)
{
    // NVS init 
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    // SPI Slave init 
    ESP_ERROR_CHECK(app_spi_slave_init());
    // Wifi and Mqtt init 
    ESP_LOGI("WIFI", "ESP_WIFI_MODE_STA");
    wifi_sta_init();
    mqtt_app_start();
    subscribe_single_topic(TOPIC);

    uint8_t frame[SPI_FRAME_BYTES];

    while(1)
    {   
        //-----------------------------------
        // get and check data when received -
        //-----------------------------------
        // delay until get total frames 
        int total_frames = app_spi_read_frame(frame, sizeof(frame), portMAX_DELAY);
        if (total_frames != SPI_FRAME_BYTES){
            ESP_LOGW(TAG, "SPI frame size mismatch (%d)", total_frames);
            continue;
        }
        // check header
        if (frame[0] != SPI_HDR0 || frame[1] != SPI_HDR1){
            ESP_LOGW(TAG, "Bad header: %02X %02X", frame[0], frame[1]);
            continue;
        }
        // get data and check for missing data 
        uint8_t seq = frame[2];
        uint16_t len = (uint16_t)frame[3] | ((uint16_t)frame[4] << 8);
        if (len != ECG_PAYLOAD_BYTES) {
            ESP_LOGW(TAG, "LEN mismatch: %u (expect %u)", len, ECG_PAYLOAD_BYTES);
            continue;
        }
        // check crc 
        uint8_t crc_recv = frame[5 + len];
        uint8_t crc_calc = app_spi_crc8(&frame[5], len);
        if (crc_recv != crc_calc) {
            ESP_LOGW(TAG, "CRC fail (seq=%u, recv=%02X calc=%02X)", seq, crc_recv, crc_calc);
            continue;
        }
        // Parse total sample received 
        const int16_t *samples = (const int16_t *)&frame[5];
        for (int i = 0; i < ECG_SAMPLES_PER_FRAME; i++) {
            arr1[i] = (float)samples[i];
            arr2[i] = arr1[i]; 
        }
        //----------------------------------------------------------------------------------
        // json init 
        char* data = create_json_two_arrays(arr1, arr2);
        if (!data) {
            ESP_LOGE(TAG, "create_json_two_arrays() returned NULL");
            continue;
        }
        publish(TOPIC, data);
        free(data);
        ESP_LOGI(TAG, "Published ECG frame seq=%u (%d samples)", seq, ECG_SAMPLES_PER_FRAME);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
