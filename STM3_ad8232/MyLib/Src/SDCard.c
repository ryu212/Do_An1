// #include "SDCard.h"

// static FATFS sd_fs;
// static bool sd_mounted = false;
// static sd_card_config_t g_sd_config;
// static char g_sd_path[8] = "0:";

// // select sd card
// static void sd_select(void)
// {
//     HAL_GPIO_WritePin(g_sd_config.sd_pins.cs_port, g_sd_config.sd_pins.cs_pin, GPIO_PIN_RESET);
// }

// // deselect sd card
// static void sd_deselect(void)
// {
//     HAL_GPIO_WritePin(g_sd_config.sd_pins.cs_port, g_sd_config.sd_pins.cs_pin, GPIO_PIN_SET);
// }

// // check if sd card is mounted
// bool sd_card_is_mounted(void)
// {
//     return sd_mounted;
// }

// // initialize sd card
// FRESULT sd_card_init(const sd_card_config_t *config)
// {
//     FRESULT res;

//     if (config == NULL)
//     {
//         return FR_INVALID_OBJECT;
//     }

//     if (sd_mounted)
//     {
//         return FR_OK;
//     }

//     g_sd_config = *config;

//     if (config->mount_point != NULL)
//     {
//         strncpy(g_sd_path, config->mount_point, sizeof(g_sd_path) - 1);
//         g_sd_path[sizeof(g_sd_path) - 1] = '\0';
//     }

//     // keep cs high before mount
//     sd_deselect();
//     HAL_Delay(10);

//     res = f_mount(&sd_fs, g_sd_path, 1);

//     if (res != FR_OK)
//     {
//         return res;
//     }

//     sd_mounted = true;
//     return FR_OK;
// }

// // deinitialize sd card
// FRESULT sd_card_deinit(void)
// {
//     FRESULT res;

//     if (!sd_mounted)
//     {
//         return FR_OK;
//     }

//     res = f_mount(NULL, g_sd_path, 1);
//     if (res == FR_OK)
//     {
//         sd_mounted = false;
//     }

//     return res;
// }

// // get sd card size in mb
// FRESULT sd_card_get_size_mb(uint32_t *size_mb)
// {
//     FATFS *fs_ptr;
//     DWORD free_clusters;
//     DWORD total_sectors;
//     DWORD total_kb;

//     if (size_mb == NULL)
//     {
//         return FR_INVALID_OBJECT;
//     }

//     if (!sd_mounted)
//     {
//         return FR_NOT_READY;
//     }

//     if (f_getfree(g_sd_path, &free_clusters, &fs_ptr) != FR_OK)
//     {
//         return FR_DISK_ERR;
//     }

//     total_sectors = (fs_ptr->n_fatent - 2U) * fs_ptr->csize;
//     total_kb = total_sectors / 2U;
//     *size_mb = total_kb / 1024U;

//     return FR_OK;
// }

// // append data to a file
// FRESULT sd_card_append(const char *filename, const uint8_t *data, UINT size)
// {
//     FIL file;
//     FRESULT res;
//     UINT bytes_written;

//     if (!sd_mounted)
//     {
//         return FR_NOT_READY;
//     }

//     if ((filename == NULL) || (data == NULL))
//     {
//         return FR_INVALID_OBJECT;
//     }

//     res = f_open(&file, filename, FA_OPEN_ALWAYS | FA_WRITE);
//     if (res != FR_OK)
//     {
//         return res;
//     }

//     res = f_lseek(&file, f_size(&file));
//     if (res != FR_OK)
//     {
//         f_close(&file);
//         return res;
//     }

//     res = f_write(&file, data, size, &bytes_written);
//     if (res != FR_OK)
//     {
//         f_close(&file);
//         return res;
//     }

//     if (bytes_written != size)
//     {
//         f_close(&file);
//         return FR_INT_ERR;
//     }

//     res = f_sync(&file);
//     f_close(&file);

//     return res;
// }

// // overwrite data to a file
// FRESULT sd_card_overwrite(const char *filename, const uint8_t *data, UINT size)
// {
//     FIL file;
//     FRESULT res;
//     UINT bytes_written;

//     if (!sd_mounted)
//     {
//         return FR_NOT_READY;
//     }

//     if ((filename == NULL) || (data == NULL))
//     {
//         return FR_INVALID_OBJECT;
//     }

//     res = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
//     if (res != FR_OK)
//     {
//         return res;
//     }

//     res = f_write(&file, data, size, &bytes_written);
//     if (res != FR_OK)
//     {
//         f_close(&file);
//         return res;
//     }

//     if (bytes_written != size)
//     {
//         f_close(&file);
//         return FR_INT_ERR;
//     }

//     res = f_sync(&file);
//     f_close(&file);

//     return res;
// }

// // read data from a file
// FRESULT sd_card_read(const char *filename, uint8_t *buffer, UINT size, UINT *bytes_read)
// {
//     FIL file;
//     FRESULT res;
//     UINT local_bytes_read = 0;

//     if (!sd_mounted)
//     {
//         return FR_NOT_READY;
//     }

//     if ((filename == NULL) || (buffer == NULL))
//     {
//         return FR_INVALID_OBJECT;
//     }

//     res = f_open(&file, filename, FA_READ);
//     if (res != FR_OK)
//     {
//         return res;
//     }

//     res = f_read(&file, buffer, size, &local_bytes_read);
//     f_close(&file);

//     if (bytes_read != NULL)
//     {
//         *bytes_read = local_bytes_read;
//     }

//     return res;
// }

// // check if file exists
// bool sd_card_file_exists(const char *filename)
// {
//     FILINFO fno;

//     if (!sd_mounted)
//     {
//         return false;
//     }

//     if (filename == NULL)
//     {
//         return false;
//     }

//     return (f_stat(filename, &fno) == FR_OK);
// }

// // delete file
// FRESULT sd_card_delete(const char *filename)
// {
//     if (!sd_mounted)
//     {
//         return FR_NOT_READY;
//     }

//     if (filename == NULL)
//     {
//         return FR_INVALID_OBJECT;
//     }

//     return f_unlink(filename);
// }

// // create directory
// FRESULT sd_card_create_dir(const char *dirname)
// {
//     if (!sd_mounted)
//     {
//         return FR_NOT_READY;
//     }

//     if (dirname == NULL)
//     {
//         return FR_INVALID_OBJECT;
//     }

//     return f_mkdir(dirname);
// }

// // delete directory
// FRESULT sd_card_delete_dir(const char *dirname)
// {
//     if (!sd_mounted)
//     {
//         return FR_NOT_READY;
//     }

//     if (dirname == NULL)
//     {
//         return FR_INVALID_OBJECT;
//     }

//     return f_unlink(dirname);
// }