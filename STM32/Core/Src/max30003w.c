#include "max30003w.h"

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
    status = HAL_SPI_TransmitReceive(hspi, tx_buf, rx_buf, (uint16_t)(len + 1), 100);
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
                    uint16_t cs_pin,
                    GPIO_TypeDef *cs_port)
{
    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_SW_RST, 0x000000)) {
        return false;
    }

    HAL_Delay(10);
    return true;
}

bool MAX30003_Sync(SPI_HandleTypeDef *hspi,
                   uint16_t cs_pin,
                   GPIO_TypeDef *cs_port)
{
    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_SYNCH, 0x000000)) {
        return false;
    }

    return true;
}

bool MAX30003_Init(SPI_HandleTypeDef *hspi,
                   uint16_t cs_pin,
                   GPIO_TypeDef *cs_port)
{
    if (!MAX30003_Reset(hspi, cs_pin, cs_port)) {
        return false;
    }
    HAL_Delay(50);

    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_GEN, 0x081007)) {
        return false;
    }
    HAL_Delay(50);

    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_CAL, 0x720000)) {
        return false;
    }
    HAL_Delay(50);

    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_EMUX, 0x0B0000)) {
        return false;
    }
    HAL_Delay(50);

    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_ECG, 0x005000)) {  // set 512 sps
        return false;
    }
    HAL_Delay(50);

    if (!MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_RTOR1, 0x3FC600)) {
        return false;
    }
    HAL_Delay(50);

    if (!MAX30003_Sync(hspi, cs_pin, cs_port)) {
        return false;
    }
    HAL_Delay(50);

    return true;
}

bool MAX30003_set_sampling_rate(SPI_HandleTypeDef *hspi,
                                GPIO_TypeDef *cs_port,
                                uint16_t cs_pin,
                                SamplingRate rate)
{
    uint8_t reg[3] = {0};

    if (!MAX30003_ReadReg(hspi, cs_port, cs_pin, REG_CNFG_ECG, 3, reg)) {
        return false;
    }

    // modify bits in MSB according to rate
    // keep other bits unchanged
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
            // leave 0 for 512 sps according to current bit mapping assumption
            break;
    }

    {
        uint32_t value = ((uint32_t)reg[0] << 16) |
                         ((uint32_t)reg[1] << 8)  |
                         ((uint32_t)reg[2]);

        return MAX30003_WriteReg(hspi, cs_port, cs_pin, REG_CNFG_ECG, value);
    }
}

bool MAX30003_read_ECGBust(SPI_HandleTypeDef *hspi,
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

        // skip invalid ECG samples
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
    uint8_t raw[3];
    uint32_t value24;
    MAX30003_Etag_t etag;
    int32_t ecg_value;
    uint8_t retry;
    const uint8_t max_retry = 10;

    if (sample == NULL) {
        return false;
    }

    for (retry = 0; retry < max_retry; retry++) {
        if (!MAX30003_ReadReg(hspi, cs_port, cs_pin, REG_ECG_FIFO, 3, raw)) {
            return false;
        }

        value24 = ((uint32_t)raw[0] << 16) |
                  ((uint32_t)raw[1] << 8)  |
                  ((uint32_t)raw[2]);

        etag = (MAX30003_Etag_t)((value24 >> 3) & 0x07);

        // accept only valid ECG samples
        if (etag == MAX30003_ETAG_VALID ||
            etag == MAX30003_ETAG_VALID_EOF) {

            // extract 18-bit ECG data from bits [23:6]
            ecg_value = (int32_t)(value24 >> 6);

            // sign-extend 18-bit signed value to 32-bit
            if (ecg_value & (1 << 17)) {
                ecg_value |= 0xFFFC0000;
            }

            *sample = ecg_value;
            return true;
        }

        // if fifo empty or invalid fast sample, try reading again
    }

    return false;
}