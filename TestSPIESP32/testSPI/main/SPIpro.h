#ifndef SPI_PRO_H
#define SPI_PRO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define MISO GPIO_NUM_19
#define MOSI GPIO_NUM_23
#define CLK  GPIO_NUM_18
#define CS   GPIO_NUM_5

#ifdef __cplusplus
extern "C" {
#endif

void spi_master_init(void);
uint8_t* spi_sent_receive(uint8_t* send_data, int send_bytes, int receive_bytes);

#ifdef __cplusplus
}
#endif

#endif // SPI_PRO_H
