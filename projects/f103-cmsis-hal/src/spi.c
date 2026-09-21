/**
 * @file    spi.c
 * @brief   SPI1：BR=DIV16（SCK ≈ 4.5 MHz），软件 NSS，Mode0/Mode3 切换
 */

#include "gpio.h"
#include "spi.h"

SPI_HandleTypeDef hspi1;

void MX_SPI1_Init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}

uint8_t SPI1_TransferByte(uint8_t tx)
{
    uint8_t rx = 0xFFU;

    if (HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1U, HAL_MAX_DELAY) != HAL_OK) {
        return 0xFFU;
    }
    return rx;
}

static void spi1_reinit(uint32_t polarity, uint32_t phase)
{
    if ((hspi1.Init.CLKPolarity == polarity) && (hspi1.Init.CLKPhase == phase) &&
        (hspi1.State == HAL_SPI_STATE_READY)) {
        return;
    }

    if (hspi1.State != HAL_SPI_STATE_RESET) {
        (void)HAL_SPI_DeInit(&hspi1);
    }
    hspi1.Init.CLKPolarity = polarity;
    hspi1.Init.CLKPhase = phase;
    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}

void SPI1_SetMode0(void)
{
    spi1_reinit(SPI_POLARITY_LOW, SPI_PHASE_1EDGE);
}

void SPI1_SetMode3(void)
{
    spi1_reinit(SPI_POLARITY_HIGH, SPI_PHASE_2EDGE);
}

void BMP280_CsLow(void)
{
    HAL_GPIO_WritePin(BMP280_CS_PORT, BMP280_CS_PIN, GPIO_PIN_RESET);
}

void BMP280_CsHigh(void)
{
    HAL_GPIO_WritePin(BMP280_CS_PORT, BMP280_CS_PIN, GPIO_PIN_SET);
}

void LSM6DS3_CsLow(void)
{
    HAL_GPIO_WritePin(LSM6DS3_CS_PORT, LSM6DS3_CS_PIN, GPIO_PIN_RESET);
}

void LSM6DS3_CsHigh(void)
{
    HAL_GPIO_WritePin(LSM6DS3_CS_PORT, LSM6DS3_CS_PIN, GPIO_PIN_SET);
}
