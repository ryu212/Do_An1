#include "max30003w.h"

#define MAX30003_SPI_TIMEOUT 10

static void max30003_cs_low(MAX30003_SPI *dev)
{
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
}

static void max30003_cs_high(MAX30003_SPI *dev)
{
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

//****************************************************************************
void max30003_init(MAX30003_SPI *dev, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    if (dev == NULL)
    {
        return;
    }

    dev->hspi = hspi;
    dev->cs_port = cs_port;
    dev->cs_pin = cs_pin;

    max30003_cs_high(dev);
}

//****************************************************************************
HAL_StatusTypeDef max30003_readRegister(MAX30003_SPI *dev, Registers_e reg, uint32_t *data)
{
    HAL_StatusTypeDef status;
    uint8_t tx;
    uint8_t rx;

    if ((dev == NULL) || (dev->hspi == NULL) || (data == NULL))
    {
        return HAL_ERROR;
    }

    *data = 0;

    max30003_cs_low(dev);

    tx = (uint8_t)((reg << 1) | 1U);
    status = HAL_SPI_TransmitReceive(dev->hspi, &tx, &rx, 1, MAX30003_SPI_TIMEOUT);
    if (status != HAL_OK)
    {
        max30003_cs_high(dev);
        return status;
    }

    tx = 0xFF;
    status = HAL_SPI_TransmitReceive(dev->hspi, &tx, &rx, 1, MAX30003_SPI_TIMEOUT);
    if (status != HAL_OK)
    {
        max30003_cs_high(dev);
        return status;
    }
    *data |= ((uint32_t)rx << 16);

    tx = 0xFF;
    status = HAL_SPI_TransmitReceive(dev->hspi, &tx, &rx, 1, MAX30003_SPI_TIMEOUT);
    if (status != HAL_OK)
    {
        max30003_cs_high(dev);
        return status;
    }
    *data |= ((uint32_t)rx << 8);

    tx = 0xFF;
    status = HAL_SPI_TransmitReceive(dev->hspi, &tx, &rx, 1, MAX30003_SPI_TIMEOUT);
    if (status != HAL_OK)
    {
        max30003_cs_high(dev);
        return status;
    }
    *data |= (uint32_t)rx;

    max30003_cs_high(dev);

    return HAL_OK;
}

//****************************************************************************
HAL_StatusTypeDef max30003_writeRegister(MAX30003_SPI *dev, Registers_e reg, const uint32_t data)
{
    HAL_StatusTypeDef status;
    uint8_t tx;

    if ((dev == NULL) || (dev->hspi == NULL))
    {
        return HAL_ERROR;
    }

    max30003_cs_low(dev);

    tx = (uint8_t)(reg << 1);
    status = HAL_SPI_Transmit(dev->hspi, &tx, 1, MAX30003_SPI_TIMEOUT);
    if (status != HAL_OK)
    {
        max30003_cs_high(dev);
        return status;
    }

    tx = (uint8_t)((0x00FF0000U & data) >> 16);
    status = HAL_SPI_Transmit(dev->hspi, &tx, 1, MAX30003_SPI_TIMEOUT);
    if (status != HAL_OK)
    {
        max30003_cs_high(dev);
        return status;
    }

    tx = (uint8_t)((0x0000FF00U & data) >> 8);
    status = HAL_SPI_Transmit(dev->hspi, &tx, 1, MAX30003_SPI_TIMEOUT);
    if (status != HAL_OK)
    {
        max30003_cs_high(dev);
        return status;
    }

    tx = (uint8_t)(0x000000FFU & data);
    status = HAL_SPI_Transmit(dev->hspi, &tx, 1, MAX30003_SPI_TIMEOUT);
    if (status != HAL_OK)
    {
        max30003_cs_high(dev);
        return status;
    }

    max30003_cs_high(dev);

    return HAL_OK;
}