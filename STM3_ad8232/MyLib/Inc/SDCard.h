#ifndef SDCARD_H
#define SDCARD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "ff.h"
#include "sd_spi.h"

/* Error codes */
typedef enum {
    SD_OK = 0,
    SD_NOT_MOUNTED,
    SD_FILE_ERROR,
    SD_FILE_NOT_FOUND,
    SD_FILE_EXISTS,
    SD_DIR_NOT_FOUND,
    SD_INIT_ERROR,
    SD_INVALID_PARAM
} sd_status_t;

/* SPI Pin configuration passed to init */
typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} sd_spi_pins_t;

/* SD card configuration */
typedef struct {
    sd_spi_pins_t sd_pins;
    const char *mount_point;        /* FatFs drive path (e.g., "0:") */
    uint8_t max_files;
    uint32_t allocation_unit_size;
} sd_card_config_t;

sd_status_t sd_card_init(const sd_card_config_t *config);
sd_status_t sd_card_deinit(void);

// Check if SD card is mounted
bool sd_card_is_mounted(void);

// Get SD card capacity in MB
sd_status_t sd_card_get_size_mb(uint32_t *size_mb);

// Write data to file (create or overwrite)
sd_status_t sd_card_write(const char *filename, const uint8_t *data, uint32_t size);

// Append data to file (create if not exists)
sd_status_t sd_card_append(const char *filename, const uint8_t *data, uint32_t size);

// Read data from file
sd_status_t sd_card_read(const char *filename, uint8_t *buffer, uint32_t max_size, uint32_t *bytes_read);

// Read entire file into buffer
int32_t sd_card_read_file(const char *filename, uint8_t *buffer, uint32_t buffer_size);

// Write buffer to file
int32_t sd_card_write_file(const char *filename, const uint8_t *buffer, uint32_t size);

// Check if file exists
bool sd_card_file_exists(const char *filename);

// Delete file
sd_status_t sd_card_delete_file(const char *filename);

// Create directory
sd_status_t sd_card_create_dir(const char *dirname);

// Delete directory
sd_status_t sd_card_delete_dir(const char *dirname);

// Get file size
sd_status_t sd_card_get_file_size(const char *filename, uint32_t *size);

#ifdef __cplusplus
}
#endif

#endif /* SDCARD_H */
