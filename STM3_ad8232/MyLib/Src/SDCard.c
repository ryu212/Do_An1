#include "SDCard.h"
#include "main.h"

/* Static variables */
static FATFS sd_fs;
static char sd_mount_point[4] = "0:";
static bool sd_is_mounted = false;
static sd_spi_pins_t sd_spi_pins;

/**
 * Convert FatFs result to SD status
 */
static sd_status_t fresult_to_sdstatus(FRESULT res)
{
    switch (res) {
        case FR_OK:
            return SD_OK;
        case FR_NO_FILE:
        case FR_NO_PATH:
            return SD_FILE_NOT_FOUND;
        case FR_EXIST:
            return SD_FILE_EXISTS;
        case FR_NOT_READY:
            return SD_NOT_MOUNTED;
        case FR_INVALID_NAME:
        case FR_INVALID_OBJECT:
        case FR_INVALID_PARAMETER:
            return SD_INVALID_PARAM;
        default:
            return SD_FILE_ERROR;
    }
}

/**
 * Initialize SD card and mount FatFs
 */
sd_status_t sd_card_init(const sd_card_config_t *config)
{
    printf("Initializing SD card...\n");
    FRESULT res;
    if (config == NULL) {
        return SD_INVALID_PARAM;
    }
    
    /* Save SPI configuration */
    sd_spi_pins = config->sd_pins;
    
    /* Initialize SPI layer */
    sd_spi_cfg_t spi_cfg = {
        .hspi = config->sd_pins.hspi,
        .cs_port = config->sd_pins.cs_port,
        .cs_pin = config->sd_pins.cs_pin
    };
    printf("Initializing SPI interface...\n");
    sd_spi_init(&spi_cfg);
    printf("SPI interface initialized.\n");
    /* Detect SD card */
    if (sd_card_detect() != 0) {
        printf("SD card detection failed!\n");
        return SD_INIT_ERROR;
    }
    printf("SD card detected successfully.\n");
    /* Set mount point */
    if (config->mount_point != NULL) {
        strncpy(sd_mount_point, config->mount_point, sizeof(sd_mount_point) - 1);
        sd_mount_point[sizeof(sd_mount_point) - 1] = '\0';
    }
    
    /* Mount FatFs */
    printf("Mounting SD card at %s...\n", sd_mount_point);
    res = f_mount(&sd_fs, sd_mount_point, 0);
    if (res != FR_OK) {
        printf("Failed to mount SD card: %d\n", res);
        return fresult_to_sdstatus(res);
    }
    printf("SD card mounted successfully at %s\n", sd_mount_point);
    sd_is_mounted = true;
    return SD_OK;
}

/**
 * Deinitialize and unmount SD card
 */
sd_status_t sd_card_deinit(void)
{
    FRESULT res;
    
    if (!sd_is_mounted) {
        return SD_OK;
    }
    
    res = f_mount(NULL, sd_mount_point, 0);
    if (res == FR_OK) {
        sd_is_mounted = false;
    }
    
    return fresult_to_sdstatus(res);
}

/**
 * Check if SD card is mounted
 */
bool sd_card_is_mounted(void)
{
    return sd_is_mounted;
}

/**
 * Get SD card capacity in MB
 */
sd_status_t sd_card_get_size_mb(uint32_t *size_mb)
{
    FATFS *pfs;
    DWORD free_clust;
    FRESULT res;
    
    if (size_mb == NULL) {
        return SD_INVALID_PARAM;
    }
    
    if (!sd_is_mounted) {
        return SD_NOT_MOUNTED;
    }
    
    res = f_getfree(sd_mount_point, &free_clust, &pfs);
    if (res != FR_OK) {
        return fresult_to_sdstatus(res);
    }
    
    /* Calculate total size: (n_fatent - 2) * csize * 512 bytes / (1024*1024) */
    *size_mb = (uint32_t)(((pfs->n_fatent - 2) * pfs->csize) >> 11);
    
    return SD_OK;
}

/**
 * Write data to file (create or overwrite)
 */
sd_status_t sd_card_write(const char *filename, const uint8_t *data, uint32_t size)
{
    FIL file;
    UINT bytes_written;
    FRESULT res;
    
    if (filename == NULL || data == NULL) {
        return SD_INVALID_PARAM;
    }
    
    if (!sd_is_mounted) {
        return SD_NOT_MOUNTED;
    }
    
    res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        return fresult_to_sdstatus(res);
    }
    
    res = f_write(&file, data, size, &bytes_written);
    f_close(&file);
    
    if (res != FR_OK) {
        return fresult_to_sdstatus(res);
    }
    
    if (bytes_written != size) {
        return SD_FILE_ERROR; /* Incomplete write */
    }
    
    return SD_OK;
}

/**
 * Append data to file (create if not exists)
 */
sd_status_t sd_card_append(const char *filename, const uint8_t *data, uint32_t size)
{
    FIL file;
    UINT bytes_written;
    FRESULT res;
    
    if (filename == NULL || data == NULL) {
        return SD_INVALID_PARAM;
    }
    
    if (!sd_is_mounted) {
        return SD_NOT_MOUNTED;
    }
    
    res = f_open(&file, filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        return fresult_to_sdstatus(res);
    }
    
    /* Move to end of file */
    f_lseek(&file, f_size(&file));
    
    res = f_write(&file, data, size, &bytes_written);
    f_close(&file);
    
    if (res != FR_OK) {
        return fresult_to_sdstatus(res);
    }
    
    if (bytes_written != size) {
        return SD_FILE_ERROR;
    }
    
    return SD_OK;
}

/**
 * Read data from file
 */
sd_status_t sd_card_read(const char *filename, uint8_t *buffer, uint32_t max_size, uint32_t *bytes_read)
{
    FIL file;
    UINT local_bytes_read = 0;
    FRESULT res;
    
    if (filename == NULL || buffer == NULL) {
        return SD_INVALID_PARAM;
    }
    
    if (!sd_is_mounted) {
        return SD_NOT_MOUNTED;
    }
    
    res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) {
        return fresult_to_sdstatus(res);
    }
    
    res = f_read(&file, buffer, max_size, &local_bytes_read);
    f_close(&file);
    
    if (bytes_read != NULL) {
        *bytes_read = local_bytes_read;
    }
    
    return fresult_to_sdstatus(res);
}

/**
 * Read entire file into buffer with simple interface
 */
int32_t sd_card_read_file(const char *filename, uint8_t *buffer, uint32_t buffer_size)
{
    FIL file;
    UINT bytes_read = 0;
    FRESULT res;
    
    if (filename == NULL || buffer == NULL) {
        return -1;
    }
    
    if (!sd_is_mounted) {
        return -1;
    }
    
    res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) {
        return -1;
    }
    
    res = f_read(&file, buffer, buffer_size, &bytes_read);
    f_close(&file);
    
    if (res != FR_OK) {
        return -1;
    }
    
    return (int32_t)bytes_read;
}

/**
 * Write buffer to file with simple interface
 */
int32_t sd_card_write_file(const char *filename, const uint8_t *buffer, uint32_t size)
{
    FIL file;
    UINT bytes_written = 0;
    FRESULT res;
    
    if (filename == NULL || buffer == NULL) {
        return -1;
    }
    
    if (!sd_is_mounted) {
        return -1;
    }
    
    res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        return -1;
    }
    
    res = f_write(&file, buffer, size, &bytes_written);
    f_close(&file);
    
    if (res != FR_OK) {
        return -1;
    }
    
    return (int32_t)bytes_written;
}

/**
 * Check if file exists
 */
bool sd_card_file_exists(const char *filename)
{
    FILINFO fno;
    
    if (filename == NULL) {
        return false;
    }
    
    if (!sd_is_mounted) {
        return false;
    }
    
    return (f_stat(filename, &fno) == FR_OK);
}

/**
 * Delete file
 */
sd_status_t sd_card_delete_file(const char *filename)
{
    if (filename == NULL) {
        return SD_INVALID_PARAM;
    }
    
    if (!sd_is_mounted) {
        return SD_NOT_MOUNTED;
    }
    
    return fresult_to_sdstatus(f_unlink(filename));
}

/**
 * Create directory
 */
sd_status_t sd_card_create_dir(const char *dirname)
{
    if (dirname == NULL) {
        return SD_INVALID_PARAM;
    }
    
    if (!sd_is_mounted) {
        return SD_NOT_MOUNTED;
    }
    
    return fresult_to_sdstatus(f_mkdir(dirname));
}

/**
 * Delete directory
 */
sd_status_t sd_card_delete_dir(const char *dirname)
{
    if (dirname == NULL) {
        return SD_INVALID_PARAM;
    }
    
    if (!sd_is_mounted) {
        return SD_NOT_MOUNTED;
    }
    
    return fresult_to_sdstatus(f_unlink(dirname));
}

/**
 * Get file size
 */
sd_status_t sd_card_get_file_size(const char *filename, uint32_t *size)
{
    FILINFO fno;
    
    if (filename == NULL || size == NULL) {
        return SD_INVALID_PARAM;
    }
    
    if (!sd_is_mounted) {
        return SD_NOT_MOUNTED;
    }
    
    FRESULT res = f_stat(filename, &fno);
    if (res != FR_OK) {
        return fresult_to_sdstatus(res);
    }
    
    *size = fno.fsize;
    return SD_OK;
}