// #ifndef SDCARD_H
// #define SDCARD_H

// #ifdef __cplusplus
// extern "C" {
// #endif

// #include <stdio.h>
// #include <stdint.h>
// #include <stdbool.h>
// #include <stddef.h>
// #include <string.h>

// #include "fatfs.h"
// #include "ff.h"

// typedef struct
// {
//     GPIO_TypeDef *cs_port;
//     uint16_t cs_pin;
//     SPI_HandleTypeDef *hspi;
// } sd_spi_pins_t;

// // cấu hình sd card cho stm32
// typedef struct
// {
//     sd_spi_pins_t sd_pins;
//     const char *mount_point;
//     uint8_t max_files;
//     uint32_t allocation_unit_size;
// } sd_card_config_t;

// // khởi tạo sd card
// FRESULT sd_card_init(const sd_card_config_t *config);

// // giải phóng sd card
// FRESULT sd_card_deinit(void);

// // kiểm tra sd card đã mount chưa
// bool sd_card_is_mounted(void);

// // lấy dung lượng sd card theo mb
// FRESULT sd_card_get_size_mb(uint32_t *size_mb);

// // ghi nối tiếp dữ liệu vào cuối file
// FRESULT sd_card_append(const char *filename, const uint8_t *data, UINT size);

// // ghi đè toàn bộ file
// FRESULT sd_card_overwrite(const char *filename, const uint8_t *data, UINT size);

// // đọc dữ liệu từ file
// FRESULT sd_card_read(const char *filename, uint8_t *buffer, UINT size, UINT *bytes_read);

// // kiểm tra file có tồn tại hay không
// bool sd_card_file_exists(const char *filename);

// // xóa file
// FRESULT sd_card_delete(const char *filename);

// // tạo thư mục
// FRESULT sd_card_create_dir(const char *dirname);

// // xóa thư mục
// FRESULT sd_card_delete_dir(const char *dirname);

// #ifdef __cplusplus
// }
// #endif

// #endif