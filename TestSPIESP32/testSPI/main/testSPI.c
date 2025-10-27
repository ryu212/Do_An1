#include <stdio.h>
#include <string.h>
#include "driver/spi_slave.h"
#include "esp_log.h"

#define PIN_NUM_MISO  19
#define PIN_NUM_MOSI  23
#define PIN_NUM_SCLK  18
#define PIN_NUM_CS    5

#define BUFFER_SIZE   500
static const char *TAG = "SPI_SLAVE";

void app_main(void)
{
    esp_err_t ret;

    // === 1. Cấu hình bus SPI ===
    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    // === 2. Cấu hình SPI Slave ===
    spi_slave_interface_config_t slvcfg = {
        .spics_io_num = PIN_NUM_CS,
        .flags = 0,
        .queue_size = 3,
        .mode = 0, // mode 0: CPOL=0, CPHA=0
    };

    // === 3. Cấu hình DMA (bắt buộc nếu dùng SPI lớn) ===
    ret = spi_slave_initialize(SPI2_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    // === 4. Chuẩn bị buffer ===
    WORD_ALIGNED_ATTR uint8_t recvbuf[BUFFER_SIZE];
    WORD_ALIGNED_ATTR uint8_t sendbuf[BUFFER_SIZE];
    uint16_t data16[BUFFER_SIZE/2];  

    memset(recvbuf, 0, sizeof(recvbuf));
    strcpy((char *)sendbuf, "ESP32 ready!");

    spi_slave_transaction_t t;
    memset(&t, 0, sizeof(t));

    // === 5. Vòng lặp giao tiếp ===
    while (1) {
        t.length = BUFFER_SIZE * 8;
        t.tx_buffer = sendbuf;
        t.rx_buffer = recvbuf;

        // Đợi master truyền dữ liệu
        ret = spi_slave_transmit(SPI2_HOST, &t, portMAX_DELAY);
        
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Expected: %d bytes, Got: %d bytes",
                BUFFER_SIZE, t.trans_len / 8);
            ESP_LOGI(TAG, "Received data (hex):");
            for (int i = 0; i < (BUFFER_SIZE >> 1); i++) {
                // Mỗi phần tử 16-bit gồm 2 byte liên tiếp (little endian)
                data16[i] = ((uint16_t)recvbuf[2*i + 1] << 8) | recvbuf[2*i];
            }
            
            // In thử ra console
            ESP_LOGI(TAG, "Received uint16_t data:");
            for (int i = 0; i < (BUFFER_SIZE >> 1); i++) {
                printf("%04X ", data16[i]);
            }
            printf("\n");
        }
    }
}
