#ifndef SD_SPI_H
#define SD_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "fatfs.h"
#include "cmsis_os.h"  /* For osSemaphore */

/* SD Card SPI responses */
#define SD_R1_READY             0x00
#define SD_R1_IDLE_STATE        0x01
#define SD_R1_ERASE_RESET       0x02
#define SD_R1_ILLEGAL_CMD       0x04
#define SD_R1_CRC_ERR           0x08
#define SD_R1_ERASE_SEQ_ERR     0x10
#define SD_R1_ADDR_ERR          0x20
#define SD_R1_PARAM_ERR         0x40
#define SD_R1_ERROR             0x80

/* SD Card Commands */
#define SD_CMD0_GO_IDLE_STATE   0
#define SD_CMD1_SEND_OP_COND    1
#define SD_CMD6_SWITCH_FUNC     6
#define SD_CMD8_SEND_IF_COND    8
#define SD_CMD9_SEND_CSD        9
#define SD_CMD10_SEND_CID       10
#define SD_CMD12_STOP_TRANS     12
#define SD_CMD13_SEND_STATUS    13
#define SD_CMD16_SET_BLOCKLEN   16
#define SD_CMD17_READ_BLOCK     17
#define SD_CMD18_READ_MULTI     18
#define SD_CMD23_SET_BLK_CNT    23
#define SD_CMD24_WRITE_BLOCK    24
#define SD_CMD25_WRITE_MULTI    25
#define SD_CMD27_PROG_CSD       27
#define SD_CMD28_SET_WR_PROT    28
#define SD_CMD29_CLR_WR_PROT    29
#define SD_CMD30_SEND_WR_PROT   30
#define SD_CMD32_ERASE_WR_BLK_START  32
#define SD_CMD33_ERASE_WR_BLK_END    33
#define SD_CMD35_ERASE_SECTOR_START  35
#define SD_CMD36_ERASE_SECTOR_END    36
#define SD_CMD37_ERASE          37
#define SD_CMD38_ERASE          38
#define SD_CMD55_APP_CMD        55
#define SD_CMD58_READ_OCR       58
#define SD_CMD59_CRC_ON_OFF     59

/* ACMD */
#define SD_ACMD6_SET_BUS_WIDTH  6
#define SD_ACMD13_SD_STATUS     13
#define SD_ACMD22_SEND_NUM_WR_BLOCKS  22
#define SD_ACMD23_SET_WR_BLK_ERASE_CNT 23
#define SD_ACMD41_SD_SEND_OP_COND     41

/* SD Card types */
typedef enum {
    SD_TYPE_UNKNOWN = 0,
    SD_TYPE_MMC,        /* MultiMediaCard */
    SD_TYPE_SDv1,       /* Standard SD v1.x */
    SD_TYPE_SDv2,       /* Standard SD v2.0 */
    SD_TYPE_SDHC,       /* SD High Capacity */
    SD_TYPE_SDXC        /* SD Extended Capacity */
} sd_card_type_t;

/* SD Card info */
typedef struct {
    sd_card_type_t type;  
    uint32_t capacity;     /* Total device size calculated in Sectors (1 Sector = 512 Bytes) */
    uint8_t csd[16];       /* Card-Specific Data: Storage capacity, read/write voltage, and speed */
    uint8_t cid[16];       /* Card Identification: Manufacturer ID, Serial Number, and Date */
} sd_card_info_t;

/* SPI pin configuration */
typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} sd_spi_cfg_t;

/* Initialize SD SPI interface */
void sd_spi_init(const sd_spi_cfg_t *cfg);

/* SPI Interrupt callbacks (called from HAL_SPI_IRQHandler) */
void sd_spi_rx_complete_callback(SPI_HandleTypeDef *hspi);
void sd_spi_tx_complete_callback(SPI_HandleTypeDef *hspi);
void sd_spi_error_callback(SPI_HandleTypeDef *hspi);

/* Select/Deselect SD card via CS */
void sd_spi_select(void);
void sd_spi_deselect(void);

/* SPI Transfer */
uint8_t sd_spi_xfer(uint8_t byte);
void sd_spi_xfer_block(uint8_t *buf, uint16_t len, bool is_read);

/* SD Card Commands */
uint8_t sd_cmd_send(uint8_t cmd, uint32_t arg, uint8_t crc);
uint8_t sd_acmd_send(uint8_t cmd, uint32_t arg);
int sd_read_response_r1(void);
int sd_read_response_r3(uint8_t *ocr);
int sd_read_data_block(uint8_t *buf, uint16_t len);
int sd_write_data_block(const uint8_t *buf, uint16_t len, uint8_t token);

/* Initialize and detect SD card */
int sd_card_detect(void);
sd_card_type_t sd_card_get_type(void);
uint32_t sd_card_get_capacity(void);

#ifdef __cplusplus
}
#endif

#endif /* SD_SPI_H */
