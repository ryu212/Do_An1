#ifndef SPI_SLAVE_DRIVER_H
#define SPI_SLAVE_DRIVER_H

#include <stdint.h>
#include "esp_err.h"
#include "Pin.h"
#define BUFFER_SIZE   720
extern QueueHandle_t mqtt_data_queue;
extern QueueHandle_t sd_card_data_queue;

// Hàm khởi tạo SPI slave
esp_err_t spi_slave_init(void);

// Hàm xử lý giao tiếp (blocking)
void spi_slave_task(void *arg);
void init_spi_queue(void);
#endif // SPI_SLAVE_DRIVER_H