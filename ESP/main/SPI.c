#include "SPI.h"
#include "string.h"
#include "esp_log.h"
#include "Port_info.h"

#define TAG "SPI_SLAVE"
// to force the starting address to be a number divisible by 4
#define DMA_ALIGN __attribute__((aligned(4)))

static spi_slave_transaction_t s_trans[SPI_QUEUE_LEN];
// buffer recieved data
static uint8_t DMA_ALIGN s_rxbuf[SPI_QUEUE_LEN][SPI_FRAME_BYTES];

static bool s_inited = false;


esp_err_t app_spi_slave_init(void){

    if (s_inited) return ESP_OK;
    // config bus data
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SPI_PIN_MOSI,
        .miso_io_num = SPI_PIN_MISO,
        .sclk_io_num = SPI_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1, 
        .max_transfer_sz = SPI_FRAME_BYTES,
        .flags = SPICOMMON_BUSFLAG_SCLK | SPICOMMON_BUSFLAG_MISO | SPICOMMON_BUSFLAG_MOSI
    };

    // coonfig slave 
    spi_slave_interface_config_t slv_cfg = {
        .spics_io_num = SPI_PIN_CS,
        .flags = 0,
        .queue_size = SPI_QUEUE_LEN,
        .mode = 0
    }; 
    ESP_ERROR_CHECK(spi_slave_initialize(SPI3_HOST, &bus_cfg, &slv_cfg, SPI_DMA_CH_AUTO));

    // delete all mem
    memset(s_trans, 0, sizeof(s_trans));
    for (int i = 0; i < SPI_QUEUE_LEN; i++) {
        // clear mem containg import data
        memset(s_rxbuf[i], 0, SPI_FRAME_BYTES);
        s_trans[i].length    = SPI_FRAME_BYTES * 8; 
        s_trans[i].rx_buffer = s_rxbuf[i];
        // import transaction into queue to send data 
        ESP_ERROR_CHECK(spi_slave_queue_trans(SPI3_HOST, &s_trans[i], portMAX_DELAY));
    }
    s_inited = true;
    ESP_LOGI(TAG, "Slave ready: frame=%d, payload=%d", SPI_FRAME_BYTES, ECG_PAYLOAD_BYTES);
    return ESP_OK;
}

int app_spi_read_frame(uint8_t *dst, size_t dst_size, TickType_t timeout){
    if (!s_inited || !dst || dst_size < SPI_FRAME_BYTES) return -1;

    // init address pointer of transaction 
    spi_slave_transaction_t *ret_trans = NULL;
    // get transaction 
    esp_err_t result = spi_slave_get_trans_result(SPI3_HOST, &ret_trans, timeout);
    if (result != ESP_OK || ret_trans == NULL) return -2;

    memcpy(dst, ret_trans->rx_buffer, SPI_FRAME_BYTES);
    // Re-queue
    ESP_ERROR_CHECK(spi_slave_queue_trans(SPI3_HOST, ret_trans, portMAX_DELAY));
    return (int)SPI_FRAME_BYTES;
}

// checksum 
uint8_t app_spi_crc8(const uint8_t *data, uint16_t len){
    uint8_t crc = 0x00;
    for (uint16_t i = 0; i < len; i++){
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++){
            crc = (crc & 0x80) ? ((crc<<1)^0x07) : (crc<<1);
        }
    }
    return crc;
}

void app_spi_deinit(void){
    if (!s_inited) return;
    spi_slave_free(SPI3_HOST);
    s_inited = false;
}


