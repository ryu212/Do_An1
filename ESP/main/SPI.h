#ifndef APP_SPI_H
#define APP_SPI_H

#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/spi_slave.h"
#include "Spi_info.h"
#ifdef __cplusplus
extern "C" {
#endif

// init SPI slave (VSPI/SPI3_HOST), mode 0

esp_err_t app_spi_slave_init(void);
void app_spi_deinit(void);
int app_spi_read_frame(uint8_t *dst, size_t dst_size, TickType_t timeout);
uint8_t app_spi_crc8(const uint8_t *data, uint16_t len);


#ifdef __cplusplus
}
#endif
#endif // APP_SPI_H
