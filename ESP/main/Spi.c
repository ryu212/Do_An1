#include "driver/spi_slave.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include "Spi.h"
#define PIN_NUM_MISO  19
#define PIN_NUM_MOSI  23
#define PIN_NUM_SCLK  18
#define PIN_NUM_CS    5
#define ELEMENTS (BUFFER_SIZE/2) // số uint16_t


static const char *TAG = "SPI_SLAVE";
QueueHandle_t spi_data_queue;

WORD_ALIGNED_ATTR static uint8_t recvbuf[BUFFER_SIZE];
WORD_ALIGNED_ATTR static uint8_t sendbuf[BUFFER_SIZE];
static uint16_t data16[BUFFER_SIZE/2];
static float data[BUFFER_SIZE/2];
esp_err_t spi_slave_init(void)
{
    esp_err_t ret;

    // 1. Cấu hình bus SPI
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    // 2. Cấu hình SPI slave
    spi_slave_interface_config_t slvcfg = {
        .spics_io_num = PIN_NUM_CS,
        .flags = 0,
        .queue_size = 3,
        .mode = 0,
    };

    // 3. Initialize SPI slave
    ret = spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_slave_initialize failed: %d", ret);
        return ret;
    }

    // 4. Chuẩn bị buffer
    memset(recvbuf, 0, sizeof(recvbuf));
    strcpy((char *)sendbuf, "ESP32 ready!");
    return ESP_OK;
}
void spi_slave_task(void *arg)
{
    spi_slave_transaction_t t;
    memset(&t, 0, sizeof(t));

    while (1) {
        t.length = BUFFER_SIZE * 8;
        t.tx_buffer = sendbuf;
        t.rx_buffer = recvbuf;

        esp_err_t ret = spi_slave_transmit(SPI2_HOST, &t, portMAX_DELAY);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Expected: %d bytes, Got: %d bytes",
                     BUFFER_SIZE, t.trans_len / 8);

            // Chuyển dữ liệu sang uint16_t
            for (int i = 0; i < (BUFFER_SIZE >> 1); i++) {
                data16[i] = ((uint16_t)recvbuf[2*i + 1] << 8) | recvbuf[2*i];
            }
            for (int i = 0; i< (BUFFER_SIZE >> 1); i++ )
            {
                data[i] = (float)((data16[i]+0.0)/0xFFF);
            }

            // In ra console
            ESP_LOGI(TAG, "Received uint16_t data:");
            for (int i = 0; i < (BUFFER_SIZE >> 1); i++) {
                printf("%04X ", data16[i]);
            }
            printf("\n");
            xQueueSend(spi_data_queue, data, portMAX_DELAY);
            ESP_LOGI(TAG,"pushed into queue\n");
        }
    }
}

void init_spi_queue(void) {
    // Tạo queue 5 phần tử, mỗi phần tử là 250 uint16_t
    spi_data_queue = xQueueCreate(5, sizeof(float) * ELEMENTS);
    
    if (spi_data_queue == NULL) {
        ESP_LOGE("QUEUE", "Tao queue that bai!");
    } else {
        ESP_LOGI("QUEUE", "Queue tao thanh cong");
    }
}