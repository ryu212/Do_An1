#ifndef MAX30003W_H
#define MAX30003W_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdbool.h>
/*
 SPI command frame structure when communicating with the MAX30003
 on the MAX30003WING board (1 transaction = 4 bytes = 32 SCLK cycles)

 Conditions:
   - Pull CSB/SSEL low before starting the transaction
   - Keep CSB/SSEL low during the entire 4-byte transfer
   - Pull CSB/SSEL high to terminate the transaction

 Byte 0 (command):
   - Consists of a 7-bit register address A[6:0] and a 1-bit R/W flag
   - R/W = 1: read
   - R/W = 0: write
   - Command format: cmd = (addr << 1) | rw

 Bytes 1..3 (24-bit data, MSB first):
   - Register write: MOSI transmits data[23:16], data[15:8], data[7:0]
   - Register read: MOSI transmits 3 dummy bytes (0x00) to generate clock,
    while MISO returns the 24-bit data in the last three bytes (rx[1], rx[2],
 rx[3])
*/
enum : uint8_t {
  // no-operation register at address 0x00
  // used when you want to clock the SPI bus without affecting internal device
  // state
  // this is only the 7-bit register address, not the final SPI command byte
  // final command byte example:
  //   read  -> (REG_NO_OP << 1) | 1
  //   write -> (REG_NO_OP << 1) | 0
  REG_NO_OP = 0x00,

  // status register
  // read this register to check interrupt/event flags and device status bits
  // commonly read by MCU after INTB/INT2B is asserted
  REG_STATUS = 0x01,

  // interrupt enable register 1
  // used to enable or mask main interrupt sources
  // usually written during device initialization
  REG_EN_INT = 0x02,

  // interrupt enable register 2
  // additional interrupt enable/mask bits
  REG_EN_INT2 = 0x03,

  // interrupt manager register
  // controls how interrupt behavior is generated/latched/routed
  REG_MNGR_INT = 0x04,

  // dynamic mode manager register
  // controls some automatic/dynamic operating behaviors of the device
  REG_MNGR_DYN = 0x05,

  // software reset command register
  // writing this register with 24-bit data = 0x000000 resets the device
  // after reset, device configuration returns to default state
  REG_SW_RST = 0x08,

  // synchronization command register
  // used to restart/synchronize ECG processing timing and internal digital path
  // commonly issued after configuration is complete
  REG_SYNCH = 0x09,

  // FIFO reset command register
  // clears/reset FIFO contents without doing a full software reset
  // useful if FIFO overflows or you want to restart fresh data capture
  REG_FIFO_RST = 0x0A,

  // information register
  // read-only register containing device identification / revision information
  REG_INFO = 0x0F,

  // general configuration register
  // contains global operating settings for the MAX30003
  // one of the most important setup registers during initialization
  REG_CNFG_GEN = 0x10,

  // calibration configuration register
  // used to configure internal calibration/test signal behavior
  REG_CNFG_CAL = 0x12,

  // electrode input multiplexer configuration register
  // selects input routing / mux behavior for ECG front end
  REG_CNFG_EMUX = 0x14,

  // ECG channel configuration register
  // configures ECG path settings such as gain, sample rate, filters, etc.
  REG_CNFG_ECG = 0x15,

  // R-to-R detector configuration register 1
  // used for heart-rate / beat-detection related settings
  REG_CNFG_RTOR1 = 0x1D,

  // R-to-R detector configuration register 2
  // second register for R-to-R detection tuning
  REG_CNFG_RTOR2 = 0x1E,

  // ECG FIFO burst register
  // used for burst reads of FIFO ECG samples
  // efficient when reading multiple FIFO words in one sequence
  REG_ECG_FIFO_BURST = 0x20,

  // ECG FIFO data register
  // read this register to get ECG sample data from FIFO
  REG_ECG_FIFO = 0x21,

  // R-to-R interval register
  // read this register to get detected R-R timing information
  REG_RTOR = 0x25,

  // alternate no-operation register at address 0x7F
  // datasheet provides both 0x00 and 0x7F as no-op addresses
  // useful as a harmless dummy access in SPI transactions
  REG_NO_OP_ALT = 0x7F
};

// Sampling rates supported
enum SamplingRate : uint16_t { SR_128 = 128, SR_256 = 256, SR_512 = 512 };

/*Create byte cmd for max30003w*/
#define MAX30003W_CREATE_CMD(addr, rw)                                         \
  ((uint8_t)(((addr) << 1) | ((rw) & 0x01)))


// Write register. Returns true on success.
bool MAX30003_WriteReg(HAL_SPI_HandleTypeDef *hspi, 
                        GPIO_TypeDef *cs_port,
                        uint8_t cs_pin,
                        uint8_t reg, uint32_t data);
// read register. Returns true on success.                     
bool MAX30003_ReadReg(HAL_SPI_HandleTypeDef *hspi, 
                        GPIO_TypeDef *cs_port,
                        uint8_t cs_pin,
                        uint8_t reg, 
                        size_t len,
                        uint8_t *buff);
// restart ECG process cleanly                       
bool MAX30003_Sync(HAL_SPI_HandleTypeDef *hspi, 
                    uint8_t cs_pin,
                    GPIO_TypeDef *cs_port);
// Reset max30003w. Returns true on success.
bool MAX30003_Reset(HAL_SPI_HandleTypeDef *hspi, 
                    uint8_t cs_pin,
                    GPIO_TypeDef *cs_port);
// initialize max30003w
bool MAX30003_Init(HAL_SPI_HandleTypeDef *hspi, uint8_t cs_pin,GPIO_TypeDef *cs_port);
// set sample rate
bool MAX30003_set_sampling_rate(HAL_SPI_HandleTypeDef *hspi, 
                                GPIO_TypeDef *cs_port, 
                                uint8_t cs_pin,
                                SamplingRate rate);
// Read many samples 
bool MAX30003_read_ECGBust(HAL_SPI_HandleTypeDef *hspi, 
                                GPIO_TypeDef *cs_port, 
                                uint16_t count, 
                                uint8_t cs_pin,
                                uint8_t *out);
// Read single ECG 24-bit signed sample. Returns true on success.
bool MAX30003_ReadECGSample(HAL_SPI_HandleTypeDef *hspi, 
                            GPIO_TypeDef *cs_port,
                            uint8_t cs_pin,
                            int32_t &sample);
#ifdef __cplusplus
}
#endif
#endif // MAX30003W_H
