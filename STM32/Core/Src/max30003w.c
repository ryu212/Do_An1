#include "max30003w.h"

static void MAX30003_Delay(uint32_t ms)
{
    if (osKernelGetState() == osKernelRunning) {
        osDelay(ms);
    } else {
        HAL_Delay(ms);
    }
}

bool MAX30003_WriteReg(SPI_HandleTypeDef *hspi,
                       GPIO_TypeDef *cs_port,
                       uint16_t cs_pin,
                       uint8_t reg,
                       uint32_t data)
{
    HAL_StatusTypeDef status;
    uint8_t tx_buf[4];

    tx_buf[0] = MAX30003W_CREATE_CMD(reg, 0);
    tx_buf[1] = (uint8_t)((data >> 16) & 0xFF);
    tx_buf[2] = (uint8_t)((data >> 8) & 0xFF);
    tx_buf[3] = (uint8_t)(data & 0xFF);

    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
    status = HAL_SPI_Transmit(hspi, tx_buf, 4, 100);
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);

    return (status == HAL_OK);
}

bool MAX30003_ReadReg(SPI_HandleTypeDef *hspi,
                      GPIO_TypeDef *cs_port,
                      uint16_t cs_pin,
                      uint8_t reg,
                      size_t len,
                      uint8_t *buff)
{
    HAL_StatusTypeDef status;
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4] = {0};

    if (buff == NULL || len == 0 || len > 3) {
        return false;
    }

    tx_buf[0] = MAX30003W_CREATE_CMD(reg, 1);

    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
    status = HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, (uint16_t)(len + 1U), 100);
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);

    if (status != HAL_OK) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        buff[i] = rx_buf[i + 1];
    }

    return true;
}

bool MAX30003_Reset(SPI_HandleTypeDef *hspi,
                    GPIO_TypeDef *cs_port,
                    uint16_t cs_pin)
{
    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_SW_RST, 0x000000)) {
        return false;
    }

    MAX30003_Delay(10);
    return true;
}

bool MAX30003_Sync(SPI_HandleTypeDef *hspi,
                   GPIO_TypeDef *cs_port,
                   uint16_t cs_pin)
{
    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_SYNCH, 0x000000)) {
        return false;
    }

    return true;
}

bool MAX30003_FIFO_Reset(SPI_HandleTypeDef *hspi,
                         GPIO_TypeDef *cs_port,
                         uint16_t cs_pin)
{
    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_FIFO_RST, 0x000000)) {
        return false;
    }

    MAX30003_Delay(10);
    return true;
}

bool MAX30003_Init(SPI_HandleTypeDef *hspi,
                   GPIO_TypeDef *cs_port,
                   uint16_t cs_pin)
{
    if (!MAX30003_Reset(hspi, cs_port, cs_pin)) {
        return false;
    }
    MAX30003_Delay(10);

    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_GEN, 0x081007)) {
        return false;
    }
    MAX30003_Delay(10);

    // disable internal calibration source for real electrode ECG
    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_CAL, 0x000000)) {
        return false;
    }
    MAX30003_Delay(10);

    // normal ECG input mux path, not VCALP/VCALN
    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_EMUX, 0x000000)) {
        return false;
    }
    MAX30003_Delay(10);

    // ECG config, 512 sps
    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_ECG, 0x005000)) {
        return false;
    }
    MAX30003_Delay(10);

    // optional RTOR block config
    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_RTOR1, 0x3FC600)) {
        return false;
    }
    MAX30003_Delay(10);

    // clear fifo state and resync after configuration
    if (!MAX30003_FIFO_Reset(hspi, cs_port, cs_pin)) {
        return false;
    }
    MAX30003_Delay(10);

    if (!MAX30003_Sync(hspi, cs_port, cs_pin)) {
        return false;
    }
    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_EN_INT,
                           (1UL << 23) |   // EN_EINT
                           (1UL << 22) |   // EN_EOVF
                           (0x03UL)))      // INTB_TYPE = 11
    {
        return false;
    }
    MAX30003_Delay(10);

    // ngưỡng FIFO interrupt
    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_MNGR_INT,
                           (0UL << 19) |   // EFIT = 0 => 1 unread sample
                           (1UL << 2)  |   // CLR_SAMP = 1
                           (0UL << 0)))    // SAMP_IT = 00
    {
        return false;
    }
    MAX30003_Delay(50);

    return true;
}

bool MAX30003_SetSamplingRate(SPI_HandleTypeDef *hspi,
                              GPIO_TypeDef *cs_port,
                              uint16_t cs_pin,
                              SamplingRate rate)
{
    uint8_t reg[3] = {0};

    if (!MAX30003_ReadReg(hspi, cs_port, cs_pin, REG_CNFG_ECG, 3, reg)) {
        return false;
    }

    // modify sample rate bits in the MSB and keep other bits unchanged
    reg[0] &= 0x3F;

    switch (rate) {
        case SR_128:
            reg[0] |= 0x80;
            break;
        case SR_256:
            reg[0] |= 0x40;
            break;
        case SR_512:
        default:
            break;
    }

    {
        uint32_t value = ((uint32_t)reg[0] << 16) |
                         ((uint32_t)reg[1] << 8)  |
                         ((uint32_t)reg[2]);

        return MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_ECG, value);
    }
}

bool MAX30003_ReadECGBurst(SPI_HandleTypeDef *hspi,
                           GPIO_TypeDef *cs_port,
                           uint16_t cs_pin,
                           uint16_t count,
                           int32_t *out,
                           uint16_t *valid_count)
{
    HAL_StatusTypeDef status;
    uint16_t total_length;
    uint8_t tx_buf[1 + 3 * count];
    uint8_t rx_buf[1 + 3 * count];
    uint16_t out_pos = 0;

    if (out == NULL || valid_count == NULL || count == 0) {
        return false;
    }

    total_length = (uint16_t)(count * 3U);

    tx_buf[0] = MAX30003W_CREATE_CMD(REG_ECG_FIFO_BURST, 1);
    for (uint16_t i = 1; i < (uint16_t)(total_length + 1U); i++) {
        tx_buf[i] = 0x00;
    }

    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_RESET);
    status = HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, (uint16_t)(total_length + 1U), 100);
    HAL_GPIO_WritePin(cs_port, cs_pin, GPIO_PIN_SET);

    if (status != HAL_OK) {
        return false;
    }

    for (uint16_t pos = 0; pos < count; pos++) {
        uint16_t idx = (uint16_t)(1U + pos * 3U);
        uint32_t value24;
        uint8_t etag;
        int32_t sample;

        value24 = ((uint32_t)rx_buf[idx] << 16) |
                  ((uint32_t)rx_buf[idx + 1] << 8) |
                  ((uint32_t)rx_buf[idx + 2]);

        etag = (uint8_t)((value24 >> 3) & 0x07);

        // only store valid ECG samples
        if (etag != MAX30003_ETAG_VALID &&
            etag != MAX30003_ETAG_VALID_EOF) {
            continue;
        }

        // extract 18-bit ECG data from bits [23:6]
        sample = (int32_t)(value24 >> 6);

        // sign-extend 18-bit signed value to 32-bit
        if (sample & (1 << 17)) {
            sample |= 0xFFFC0000;
        }

        out[out_pos] = sample;
        out_pos++;
    }

    *valid_count = out_pos;
    return true;
}

bool MAX30003_ReadECGSample(SPI_HandleTypeDef *hspi,
                            GPIO_TypeDef *cs_port,
                            uint16_t cs_pin,
                            int32_t *sample)
{
    uint8_t raw[3] = {0};
    //int32_t value24;
    //uint8_t etag;
    int32_t ecg_value;


    if (sample == NULL) {
        return false;
    }

    if (!MAX30003_ReadReg(hspi, cs_port, cs_pin, REG_ECG_FIFO, 3, raw)) {
        return false;
    }

    ecg_value = ((uint32_t)raw[0] << 16) |
                ((uint32_t)raw[1] << 8)  |
                (raw[2]);
    if (ecg_value & 0x800000) ecg_value |= 0xFF000000;
    *sample = ecg_value;
    return true;
    // value24 = ((uint32_t)raw[0] << 16) |
    //           ((uint32_t)raw[1] << 8)  |
    //           ((uint32_t)raw[2]);

    // etag = (uint8_t)((value24 >> 3) & 0x07);
    // // printf("raw = %02X %02X %02X, etag = %u\r\n", raw[0], raw[1], raw[2], etag);

    // //fifo overflow: clear fifo and resync before returning
    // if (etag == MAX30003_ETAG_FIFO_OVERFLOW) {
    //     MAX30003_FIFO_Reset(hspi, cs_port, cs_pin);
    //     MAX30003_Sync(hspi, cs_port, cs_pin);
    //     return false;
    // }

    // // skip non-valid sample types
    // if (etag == MAX30003_ETAG_FIFO_EMPTY ||
    //     etag == MAX30003_ETAG_FAST ||
    //     etag == MAX30003_ETAG_FAST_EOF ||
    //     etag == MAX30003_ETAG_UNUSED_4 ||
    //     etag == MAX30003_ETAG_UNUSED_5) {
    //     return false;
    // }

    // // extract 18-bit ECG data from bits [23:6]
    // ecg_value = (int32_t)(value24 >> 6);

    // // sign-extend 18-bit signed value to 32-bit
    // if (ecg_value & (1 << 17)) {
    //     ecg_value |= 0xFFFC0000;
    // }
    // *sample = ecg_value;
    // return true;
}