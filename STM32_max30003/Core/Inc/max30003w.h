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

///MAX30003 Registers
typedef enum
{
    NO_OP          = 0x00,
    STATUS         = 0x01,
    EN_INT         = 0x02,
    EN_INT2        = 0x03,
    MNGR_INT       = 0x04,
    MNGR_DYN       = 0x05,
    SW_RST         = 0x08,
    SYNCH          = 0x09,
    FIFO_RST       = 0x0A,
    INFO           = 0x0F,
    CNFG_GEN       = 0x10,
    CNFG_ALL       = 0x12,
    CNFG_EMUX      = 0x14,
    CNFG_ECG       = 0x15,
    CNFG_RTOR1     = 0x1D,
    CNFG_RTOR2     = 0x1E,
    ECG_FIFO_BURST = 0x20,
    ECG_FIFO       = 0x21,
    RTOR           = 0x25,
    NO_OP2         = 0x7F
} Registers_e;

///Status register bits
typedef union
{
    ///Access all bits
    uint32_t all;
    
    ///Access individual bits
    struct
    {
        uint32_t loff_nl    : 1;
        uint32_t loff_nh    : 1;
        uint32_t loff_pl    : 1;
        uint32_t loff_ph    : 1;
        uint32_t reserved1  : 4;
        uint32_t pllint     : 1;
        uint32_t samp       : 1;
        uint32_t rrint      : 1;
        uint32_t lonint     : 1;
        uint32_t reserved2  : 8;
        uint32_t dcloffint  : 1;
        uint32_t fstint     : 1;
        uint32_t eovf       : 1;
        uint32_t eint       : 1;
        uint32_t reserved3  : 8;
    } bits;
} Status_u;

///Enable Interrupt registers bits
typedef union
{
    ///Access all bits
    uint32_t all;

    ///Access individual bits
    struct
    {
        uint32_t intb_type    : 2;
        uint32_t reserved1    : 6;
        uint32_t en_pllint    : 1;
        uint32_t en_samp      : 1;
        uint32_t en_rrint     : 1;
        uint32_t en_loint     : 1;
        uint32_t reserved2    : 8;
        uint32_t en_dcloffint : 1;
        uint32_t en_fstint    : 1;
        uint32_t en_eovf      : 1;
        uint32_t en_eint      : 1;
        uint32_t reserved3    : 8;
    } bits;
} EnableInterrupts_u;

///Manage Interrupt register bits
typedef union
{
    ///Access all bits
    uint32_t all;
    
    ///Access individual bits
    struct
    {
        uint32_t samp_it   : 4;
        uint32_t clr_samp  : 1;
        uint32_t reserved1 : 1;
        uint32_t clr_rrint : 2;
        uint32_t clr_fast  : 1;
        uint32_t reserved2 : 12;
        uint32_t efit      : 5;
        uint32_t reserved3 : 8;
    } bits;
} ManageInterrupts_u;

///Manage Dynamic Modes register bits
typedef union
{
    ///Access all bits
    uint32_t all;
    
    ///Access individual bits
    struct
    {
        uint32_t reserved1 : 16;
        uint32_t fast_th   : 6;
        uint32_t fast      : 2;
        uint32_t reserved2 : 8;
    } bits;
} ManageDynamicModes_u;

///General Configuration bits
typedef union
{
    ///Access all bits
    uint32_t all;
    
    ///Access individual bits
    struct
    {
        uint32_t rbiasn     : 1;
        uint32_t rbiasp     : 1;
        uint32_t rbiasv     : 2;
        uint32_t en_rbias   : 2;
        uint32_t vth        : 2;
        uint32_t imag       : 3;
        uint32_t ipol       : 1;
        uint32_t en_dcloff  : 2;
        uint32_t reserved1  : 5;
        uint32_t en_ecg     : 1;
        uint32_t fmstr      : 2;
        uint32_t en_ulp_lon : 2;
        uint32_t reserved2  : 8;
    } bits;
} GeneralConfiguration_u;

///Cal Configuration bits
typedef union
{
    ///Access all bits
    uint32_t all;
    
    ///Access individual bits
    struct
    {
        uint32_t thigh     : 11;
        uint32_t fifty     : 1;
        uint32_t fcal      : 3;
        uint32_t reserved1 : 5;
        uint32_t vmag      : 1;
        uint32_t vmode     : 1;
        uint32_t en_vcal   : 1;
        uint32_t reserved2 : 9;
        
    } bits;
} CalConfiguration_u;

///Mux Configuration bits
typedef union
{
    ///Access all bits
    uint32_t all;
    
    ///Access individual bits
    struct
    {
        uint32_t reserved1 : 16;
        uint32_t caln_sel  : 2;
        uint32_t calp_sel  : 2;
        uint32_t openn     : 1;
        uint32_t openp     : 1;
        uint32_t reserved2 : 1;
        uint32_t pol       : 1;
        uint32_t reserved3 : 8;
    } bits;
} MuxConfiguration_u;

///ECG Configuration bits
typedef union
{
    ///Access all bits
    uint32_t all;
    
    ///Access individual bits
    struct
    {
        uint32_t reserved1 : 12;
        uint32_t dlpf      : 2;
        uint32_t dhpf      : 1;
        uint32_t reserved2 : 1;
        uint32_t gain      : 2;
        uint32_t reserved3 : 4;
        uint32_t rate      : 2;
        uint32_t reserved4 : 8;
    } bits;
} ECGConfiguration_u;

///RtoR1 Configuration bits
typedef union
{
    ///Access all bits
    uint32_t all;
    
    ///Access individual bits
    struct
    {
        uint32_t reserved1 : 8;
        uint32_t ptsf      : 4;
        uint32_t pavg      : 2;
        uint32_t reserved2 : 1;
        uint32_t en_rtor   : 1;
        uint32_t rgain     : 4;
        uint32_t wndw      : 4;
        uint32_t reserved3 : 8;
    } bits;
} RtoR1Configuration_u;

///RtoR2 Configuration bits
typedef union
{
  ///Access all bits
  uint32_t all;
  
  ///Access individual bits
  struct
  {
      uint32_t reserved1 : 8;
      uint32_t rhsf      : 3;
      uint32_t reserved2 : 1;
      uint32_t ravg      : 2;
      uint32_t reserved3 : 2;
      uint32_t hoff      : 6;
      uint32_t reserved4 : 10;
  } bits;
} RtoR2Configuration_u;

typedef struct{
  SPI_HandleTypeDef *hspi;
  GPIO_TypeDef *cs_port;
  uint16_t cs_pin;
} MAX30003_SPI;

void max30003_init(MAX30003_SPI *dev, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);
HAL_StatusTypeDef max30003_writeRegister(MAX30003_SPI *dev, Registers_e reg, uint32_t data);
HAL_StatusTypeDef max30003_readRegister(MAX30003_SPI *dev, Registers_e reg, uint32_t *data);

#ifdef __cplusplus
}
#endif

#endif /* MAX30003W_H */