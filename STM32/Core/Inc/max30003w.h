#ifndef MAX30003W_H
#define MAX30003W_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include "cmsis_os2.h"

/*
 SPI command frame structure for MAX30003

 one transaction = 4 bytes = 32 sclk cycles

 conditions
   - pull csb/ssel low before starting the transaction
   - keep csb/ssel low during the entire 4-byte transfer
   - pull csb/ssel high to terminate the transaction

 byte 0: command
   - 7-bit register address A[6:0]
   - 1-bit r/w flag
   - rw = 1: read
   - rw = 0: write
   - command format: cmd = (addr << 1) | rw

 bytes 1..3: 24-bit data, msb first
   - register write:
       mosi sends data[23:16], data[15:8], data[7:0]
   - register read:
       mosi sends 3 dummy bytes (0x00) to generate clock
       miso returns the 24-bit register data in the last 3 bytes

 ECG FIFO word format
   - bits [23:6] : 18-bit ECG sample
   - bits [5:3]  : ETAG
   - bits [2:0]  : remaining status bits / reserved depending on word type
*/

typedef enum {
    REG_NO_OP           = 0x00,
    REG_STATUS          = 0x01,
    REG_EN_INT          = 0x02,
    REG_EN_INT2         = 0x03,
    REG_MNGR_INT        = 0x04,
    REG_MNGR_DYN        = 0x05,
    REG_SW_RST          = 0x08,
    REG_SYNCH           = 0x09,
    REG_FIFO_RST        = 0x0A,
    REG_INFO            = 0x0F,
    REG_CNFG_GEN        = 0x10,
    REG_CNFG_CAL        = 0x12,
    REG_CNFG_EMUX       = 0x14,
    REG_CNFG_ECG        = 0x15,
    REG_CNFG_RTOR1      = 0x1D,
    REG_CNFG_RTOR2      = 0x1E,
    REG_ECG_FIFO_BURST  = 0x20,
    REG_ECG_FIFO        = 0x21,
    REG_RTOR            = 0x25,
    REG_NO_OP_ALT       = 0x7F
} MAX30003_Reg_t;

/* sampling rates */
typedef enum {
    SR_128 = 128,
    SR_256 = 256,
    SR_512 = 512
} SamplingRate;

/* ECG FIFO ETAG values */
typedef enum {
    MAX30003_ETAG_VALID         = 0x00,
    MAX30003_ETAG_FAST          = 0x01,
    MAX30003_ETAG_VALID_EOF     = 0x02,
    MAX30003_ETAG_FAST_EOF      = 0x03,
    MAX30003_ETAG_UNUSED_4      = 0x04,
    MAX30003_ETAG_UNUSED_5      = 0x05,
    MAX30003_ETAG_FIFO_EMPTY    = 0x06,
    MAX30003_ETAG_FIFO_OVERFLOW = 0x07
} MAX30003_Etag_t;

/* create command byte for max30003 */
#define MAX30003W_CREATE_CMD(addr, rw) ((uint8_t)(((addr) << 1) | ((rw) & 0x01)))

/* write one 24-bit register */
bool MAX30003_WriteReg(SPI_HandleTypeDef *hspi,
                       GPIO_TypeDef *cs_port,
                       uint16_t cs_pin,
                       uint8_t reg,
                       uint32_t data);

/* read one 24-bit register */
bool MAX30003_ReadReg(SPI_HandleTypeDef *hspi,
                      GPIO_TypeDef *cs_port,
                      uint16_t cs_pin,
                      uint8_t reg,
                      size_t len,
                      uint8_t *buff);

/* software reset */
bool MAX30003_Reset(SPI_HandleTypeDef *hspi,
                    GPIO_TypeDef *cs_port,
                    uint16_t cs_pin);

/* resynchronize ECG timing / digital path */
bool MAX30003_Sync(SPI_HandleTypeDef *hspi,
                   GPIO_TypeDef *cs_port,
                   uint16_t cs_pin);

/* reset FIFO, useful after overflow */
bool MAX30003_FIFO_Reset(SPI_HandleTypeDef *hspi,
                         GPIO_TypeDef *cs_port,
                         uint16_t cs_pin);

/* initialize max30003 for normal live ECG use */
bool MAX30003_Init(SPI_HandleTypeDef *hspi,
                   GPIO_TypeDef *cs_port,
                   uint16_t cs_pin);

/* set sample rate */
bool MAX30003_SetSamplingRate(SPI_HandleTypeDef *hspi,
                              GPIO_TypeDef *cs_port,
                              uint16_t cs_pin,
                              SamplingRate rate);

/* read many ECG FIFO samples in one burst */
bool MAX30003_ReadECGBurst(SPI_HandleTypeDef *hspi,
                           GPIO_TypeDef *cs_port,
                           uint16_t cs_pin,
                           uint16_t count,
                           int32_t *out,
                           uint16_t *valid_count);

/* read one ECG FIFO word and extract signed 18-bit ECG sample */
bool MAX30003_ReadECGSample(SPI_HandleTypeDef *hspi,
                            GPIO_TypeDef *cs_port,
                            uint16_t cs_pin,
                            int32_t *sample);

#ifdef __cplusplus
}
#endif

#endif /* MAX30003W_H */