#include "max30003w.h"

bool MAX30003_WriteReg(HAL_SPI_HandleTypeDef *hspi, 
                        GPIO_TypeDef *cs_port,
                        uint8_t cs_pin,
                        uint8_t reg, uint32_t data) {
  uint8_t tx_buf[4];
  tx_buf[0] = MAX30003W_CREATE_CMD(reg, 0);
  tx_buf[1] = (data >> 16) & 0xFF;
  tx_buf[2] = (data >> 8) & 0xFF;
  tx_buf[3] = data & 0xFF;
  HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(hspi, tx_buf, 4, 100);
  HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
  return true;
}
bool MAX30003_ReadReg(HAL_SPI_HandleTypeDef *hspi, 
                        GPIO_TypeDef *cs_port,
                        uint8_t cs_pin,
                        uint8_t reg, 
                        size_t len,
                        uint8_t *buff)
{
    HAL_StatusTypeDef status;
    uint8_t header;
    uint8_t header = MAX30003W_CREATE_CMD(reg, 1);
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(hspi, &header, 1, 100);
    if (status != HAL_OK) {
        HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
        return false;
    }
    status = HAL_SPI_Receive(hspi, buff, len, 100);
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);
    return (status == HAL_OK);
}
bool MAX30003_Reset(HAL_SPI_HandleTypeDef *hspi, 
                    uint8_t cs_pin,
                    GPIO_TypeDef *cs_port) {
  if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_SW_RST, 0x000000)) {
    return false;
  }
  HAL_Delay(10);
  return true;
}
bool MAX30003_Sync(HAL_SPI_HandleTypeDef *hspi, 
                    uint8_t cs_pin,
                    GPIO_TypeDef *cs_port){
  if(!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_SYNCH, 0x000000)) return false;
  return true;
}
bool MAX30003_Init(HAL_SPI_HandleTypeDef *hspi, uint8_t cs_pin,GPIO_TypeDef *cs_port) {
    bool err;
    err = MAX30003_Reset(hspi, cs_pin, cs_port);
    HAL_Delay(50);
    err = MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_GEN, 0x081007);
    HAL_Delay(50);
    err = MAX30003_WriteReg(hspi, cs_port, cs_pin, cs_pin, REG_CNFG_CAL, 0x720000);
    HAL_Delay(50);
    err = MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_EMUX, 0x0B0000);
    HAL_Delay(50);
    err = MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_ECG, 0x005000);  // set 512 sps
    HAL_Delay(50);
    err = MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_RTOR1, 0x3FC600);
    HAL_Delay(50);
    err = MAX30003_Sync(hspi, cs_pin, cs_port);
    HAL_Delay(50);
    return err;
}
bool MAX30003_set_sampling_rate(HAL_SPI_HandleTypeDef *hspi, 
                                GPIO_TypeDef *cs_port,
                                uint8_t cs_pin, 
                                SamplingRate rate)
{
    uint8_t reg[3] = {0};
    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_ECG, 3);) return false;
    // modify bits in MSB according to rate
    reg[0] &= 0x3F; // clear sample rate bits (example mask)
    switch (rate)
    {
        case SR_128: reg[0] |= 0x80; break;
        case SR_256: reg[0] |= 0x40; break;
        case SR_512: /* leave 0 */ break;
    }
    uint32_t value = ((uint32_t)reg[0] << 16) | ((uint32_t)reg[1] << 8) | reg[2];
    return MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_ECG, value);
}     
bool MAX30003_read_ECGBust(HAL_SPI_HandleTypeDef *hspi, 
                                GPIO_TypeDef *cs_port, 
                                uint8_t cs_pin,
                                uint16_t count, 
                                uint8_t *out)                        
{   
    return MAX30003_ReadReg(hspi, cs_port, cs_pin,
                            REG_ECG_FIFO_BURST,
                            count * 3U,
                            out);
}
bool MAX30003_ReadECGSample(HAL_SPI_HandleTypeDef *hspi, 
                            GPIO_TypeDef *cs_port,
                            uint8_t cs_pin,
                            int32_t &sample)
{
    uint8_t raw[3];
    uint32_t value24;
    if (!MAX30003_ReadReg(hspi, cs_port, cs_pin, REG_ECG_FIFO, 3, raw)) {
        return false;
    }
    value24 = ((uint32_t)raw[0] << 16) |
              ((uint32_t)raw[1] << 8)  |
              raw[2];
    sample = (int32_t)(value24 >> 6);
    if (sample & (1 << 17)) {
        sample |= 0xFFFC0000;
    }
    return true;
}




