#include "SDCard.h"

static const char *TAG = "SD_CARD";
static bool sd_mounted = false;
static sdmmc_card_t *card; //variable to hold SD card information

// initialize SD card
esp_err_t sd_card_init(const sd_card_config_t *config){
    if (sd_mounted) {
        ESP_LOGW(TAG, "SD card already mounted");
        return ESP_OK;
    }
    if (config == NULL || config->mount_point == NULL) {
        ESP_LOGE(TAG, "Invalid SD card configuration");
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret;
    // Configure bus spi in master mode for SD card
    spi_bus_config_t sd_card_bus_cfg = {
        .mosi_io_num = config->sd_pins.mosi,
        .miso_io_num = config->sd_pins.miso,
        .sclk_io_num = config->sd_pins.sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = 4000,
        .flags = SPICOMMON_BUSFLAG_MASTER,
        .intr_flags = 0
    };
    esp_err_t bus_ret = spi_bus_initialize(config->sd_host_id, &sd_card_bus_cfg, SDSPI_DEFAULT_DMA);
    if (bus_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(bus_ret));
        return bus_ret;
    }
    // configure host for sdspi
    /*
    sdmmc_host_t is a structure that defines the host controller for SD card communication. 
    It includes information about the SPI host ID, 
    the maximum transfer size, 
    and other parameters needed to initialize the SD card interface. 
    In this code, we use the SDSPI_HOST_DEFAULT() macro to get a default configuration for the SD card host,
    and then we set the slot field to the value specified in the config structure passed to the sd_card_init function.
    */
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = config->sd_host_id;
    // configure slot for sdspi
    /*
    sdspi_device_config_t is a structure that defines the configuration for the SD card slot when using the SPI interface.
     */
    sdspi_device_config_t sd_card_slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    sd_card_slot_config.host_id = config->sd_host_id;
    sd_card_slot_config.gpio_cs = config->sd_pins.cs;
    // configure mount point for SD card
    /*
    role of esp_vfs_fat_sdmmc_mount_config_t is to define the configuration for mounting the SD card filesystem.
    format_if_mount_failed: This field indicates whether the SD card should be formatted if mounting the filesystem fails. 
    If set to true, the SD card will be formatted and the filesystem will be created. If set to false, 
    the mounting will fail and an error will be returned.
    if format_if_mount_failed is false that mean allocation_unit_size dont work
     */
    esp_vfs_fat_sdmmc_mount_config_t sd_card_mount_cfg = {
        .format_if_mount_failed = config->format_if_mount_failed,
        .max_files = (config->max_files == 0) ? 5 : config->max_files,
        .allocation_unit_size = (config->allocation_unit_size == 0) ? (4 * 1024) : config->allocation_unit_size,
        .disk_status_check_enable = false,
        .use_one_fat = false
    };

    // configure mount options for SD card
    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    /*
    Role is to mount the SD card filesystem through the SPI communication into the VFS (Virtual File System) of the ESP32.
    It helps to work with SPI standard ex fopen, fread, dwrite, fclose, etc. to access files on the SD card.
     */
    bus_ret = esp_vfs_fat_sdspi_mount(config->mount_point, 
                                        &host, 
                                        &sd_card_slot_config, 
                                        &sd_card_mount_cfg, 
                                        &card);
    if (bus_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card filesystem: %s", esp_err_to_name(bus_ret));
        spi_bus_free(config->sd_host_id);
        card = NULL;
        return bus_ret;
    }
    sd_mounted = true;
    ESP_LOGI(TAG, "SD card mounted successfully");
    return ESP_OK;
}

// deinitialize SD card
esp_err_t sd_card_deinit(const sd_card_config_t *config){
    if (!sd_mounted) {
        ESP_LOGW(TAG, "SD card not mounted");
        return ESP_OK;
    }
    if (config == NULL || config->mount_point == NULL) {
        ESP_LOGE(TAG, "Invalid SD card configuration");
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(config->mount_point, card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
        return ret;
    }
    spi_bus_free(config->sd_host_id);
    card = NULL;
    sd_mounted = false;
    ESP_LOGI(TAG, "SD card deinitialized successfully");
    return ESP_OK;
}

// check if SD card is mounted
bool sd_card_is_mounted(void){
    return sd_mounted;
}

// get SD card size in MB
esp_err_t sd_card_get_size_mb(size_t *size_mb){
    if (!sd_mounted) {
        ESP_LOGW(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    if (size_mb == NULL) {
        ESP_LOGE(TAG, "Invalid argument: size_mb is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    uint64_t total_bytes = ((uint64_t)card->csd.capacity) * card->csd.sector_size;
    *size_mb = (size_t)(total_bytes / (1024ULL * 1024ULL));
    return ESP_OK;
}

// append data to a file on SD card
esp_err_t sd_card_append(const char *filename, const uint8_t *data, size_t size){
    if (filename == NULL || data == NULL) {
        ESP_LOGE(TAG, "Invalid argument: filename or data is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (!sd_mounted) {
        ESP_LOGW(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    FILE *file = fopen(filename, "ab"); // ad mode to append data to the end of the file
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for appending: %s", filename);
        return ESP_ERR_NOT_FOUND;
    }
    size_t written = fwrite(data, 1, size, file);
    fclose(file);
    if (written != size) {
        ESP_LOGE(TAG, "Failed to write all data to file: %s", filename);
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}

// overwrite a file on SD card
esp_err_t sd_card_overwrite(const char *filename, const uint8_t *data, size_t size){
    if (filename == NULL || data == NULL) {
        ESP_LOGE(TAG, "Invalid argument: filename or data is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (!sd_mounted) {
        ESP_LOGW(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }
    FILE *file = fopen(filename, "wb"); 
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for appending: %s", filename);
        return ESP_ERR_NOT_FOUND;
    }
    size_t written = fwrite(data, 1, size, file);
    fclose(file);
    if (written != size) {
        ESP_LOGE(TAG, "Failed to write all data to file: %s", filename);
        return ESP_ERR_INVALID_SIZE;
    }
    return ESP_OK;
}
//check if file exists on SD card
bool sd_card_file_exists(const char *filename){
    if (filename == NULL || !sd_mounted){
        return false;
    }
    struct stat st;
    return (stat(filename, &st) == 0) && S_ISREG(st.st_mode);
}

// delete a file on SD card
esp_err_t sd_card_delete(const char *filename){
    if (filename == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (unlink(filename) != 0)
    {
        ESP_LOGE(TAG, "Failed to delete file : %s", filename);
        return ESP_FAIL;
    }
    return ESP_OK;
}
// read data from a file on SD card
esp_err_t sd_card_read(const char *filename, uint8_t *buffer, size_t size){
    if (filename == NULL || buffer == NULL) {
        ESP_LOGE(TAG, "Invalid argument: filename or buffer is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (!sd_mounted) {
        ESP_LOGW(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", filename);
        return ESP_ERR_NOT_FOUND;
    }

    // read at most the exact number of bytes requested by the function
    size_t read_bytes = fread(buffer, 1, size, file);
    fclose(file);

    // if fewer bytes were read than requested, check whether it was a read error
    // or simply because the file is shorter than the requested size
    if (read_bytes < size) {
        struct stat st;
        if (stat(filename, &st) != 0) {
            ESP_LOGE(TAG, "Failed to stat file after reading: %s", filename);
            return ESP_FAIL;
        }

        // if the file still has enough data but fread read less than requested,
        // then treat it as a read failure
        if ((size_t)st.st_size >= size) {
            ESP_LOGE(TAG, "Failed to read all requested data from file: %s", filename);
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}

// create a directory on SD card
esp_err_t sd_card_create_dir(const char *dirname){
    if (dirname == NULL) {
        ESP_LOGE(TAG, "Invalid argument: dirname is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (!sd_mounted) {
        ESP_LOGW(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    // if the directory already exists, do not treat it as an error
    struct stat st;
    if (stat(dirname, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return ESP_OK;
        }
        ESP_LOGE(TAG, "Path exists but is not a directory: %s", dirname);
        return ESP_FAIL;
    }

    // create the directory with normal read/write permissions
    if (mkdir(dirname, 0775) != 0) {
        ESP_LOGE(TAG, "Failed to create directory: %s", dirname);
        return ESP_FAIL;
    }

    return ESP_OK;
}

// delete a directory on SD card
esp_err_t sd_card_delete_dir(const char *dirname){
    if (dirname == NULL) {
        ESP_LOGE(TAG, "Invalid argument: dirname is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    if (!sd_mounted) {
        ESP_LOGW(TAG, "SD card not mounted");
        return ESP_ERR_INVALID_STATE;
    }

    struct stat st;
    if (stat(dirname, &st) != 0) {
        ESP_LOGE(TAG, "Directory not found: %s", dirname);
        return ESP_ERR_NOT_FOUND;
    }
    if (!S_ISDIR(st.st_mode)) {
        ESP_LOGE(TAG, "Path is not a directory: %s", dirname);
        return ESP_ERR_INVALID_ARG;
    }

    // only remove an empty directory to avoid adding recursive delete logic
    // beyond the current requirement
    if (rmdir(dirname) != 0) {
        ESP_LOGE(TAG, "Failed to delete directory: %s", dirname);
        return ESP_FAIL;
    }

    return ESP_OK;
}