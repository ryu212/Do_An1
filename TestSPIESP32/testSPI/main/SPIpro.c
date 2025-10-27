#include "SPIpro.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include <stdlib.h>
#include <string.h>

#define TAG "SPI"

#define MISO GPIO_NUM_19
#define MOSI GPIO_NUM_23
#define CLK  GPIO_NUM_18
#define CS   GPIO_NUM_5

spi_device_handle_t spi = NULL;

void spi_master_init(void)
{
    esp_err_t ret;

    spi_bus_config_t buscfg = {
        .miso_io_num = MISO,
        .mosi_io_num = MOSI,
        .sclk_io_num = CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    // Khởi tạo bus SPI với SPI2_HOST
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "SPI bus initialized");

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 100,  // 1 MHz
        .mode = 0,                          // SPI mode 0
        .spics_io_num = CS,
        .queue_size = 1,
    };

    // Gán thiết bị SPI vào bus
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "SPI device added");
}

uint8_t* spi_sent_receive(uint8_t* send_data, int send_bytes, int receive_bytes)
{
    esp_err_t ret;

    uint8_t* receive_buffer = NULL;
    if (receive_bytes > 0) {
        receive_buffer = malloc(receive_bytes);
        if (!receive_buffer) {
            ESP_LOGE(TAG, "Failed to allocate memory for receive_buffer");
            return NULL;
        }
    }

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.length = send_bytes * 8;
    t.rxlength = receive_bytes * 8;
    t.tx_buffer = send_data;
    t.rx_buffer = receive_buffer;

    // Dùng truyền half-duplex nếu cần
    // t.flags = SPI_DEVICE_HALFDUPLEX;

    ret = spi_device_transmit(spi, &t);
    ESP_ERROR_CHECK(ret);

    if (receive_bytes && ret == ESP_OK) {
        ESP_LOGI(TAG, "Received:");
        for (int i = 0; i < receive_bytes; i++) {
            printf("0x%02X ", receive_buffer[i]);
        }
        printf("\n");
    }

    return receive_buffer;
}
