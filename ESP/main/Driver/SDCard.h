#ifndef SDCard_H
#define SDCard_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_err.h"
#include "esp_log.h"

#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "driver/gpio.h"

#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"


typedef struct {
    gpio_num_t mosi;
    gpio_num_t miso;
    gpio_num_t sclk;
    gpio_num_t cs;
} sd_spi_pins_t;

// SD card configuration structure
typedef struct {
    sd_spi_pins_t sd_pins;     
    spi_host_device_t  sd_host_id;  // SPI host ID 
    const char *mount_point; // namefile mount point for SD card
    bool format_if_mount_failed; // if is is true value, SD card will be overwritten if mounting fails
    uint8_t max_files; // maximum number of open files
    uint32_t allocation_unit_size; // allocation unit size for SD card
} sd_card_config_t;

// initialize SD card
esp_err_t sd_card_init(const sd_card_config_t *config);

// deinitialize SD card
esp_err_t sd_card_deinit(const sd_card_config_t *config);

// check if SD card is mounted
bool sd_card_is_mounted(void);

// get SD card size in MB
esp_err_t sd_card_get_size_mb(size_t *size_mb);    

// append data to a file on SD card
esp_err_t sd_card_append(const char *filename, const uint8_t *data, size_t size);

// overwrite a file on SD card
esp_err_t sd_card_overwrite(const char *filename, const uint8_t *data, size_t size);

// // read data from a file on SD card
// esp_err_t sd_card_read(const char *filename, uint8_t *buffer, size_t size);

//check if file exists on SD card
bool sd_card_file_exists(const char *filename);

// delete a file on SD card
esp_err_t sd_card_delete(const char *filename);

// // create a directory on SD card
// esp_err_t sd_card_create_dir(const char *dirname);

// // delete a directory on SD card
// esp_err_t sd_card_delete_dir(const char *dirname);

#ifdef __cplusplus
}
#endif
#endif