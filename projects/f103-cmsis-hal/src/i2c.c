/**
 * @file    i2c.c
 * @brief   I2C1 400 kHz Fast 主机；页写走轮询（API 名保留 WriteDma）
 */

#include "i2c.h"

I2C_HandleTypeDef hi2c1;

void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}

uint8_t I2C1_Write(uint8_t addr8, const uint8_t *buf, uint32_t len)
{
    if ((buf == 0) || (len == 0U)) {
        return 0U;
    }
    if (HAL_I2C_Master_Transmit(&hi2c1, addr8, (uint8_t *)buf, (uint16_t)len, 100U) != HAL_OK) {
        return 0U;
    }
    return 1U;
}

uint8_t I2C1_WriteDma(uint8_t addr8, uint8_t ctrl, const uint8_t *mem, uint32_t len)
{
    static uint8_t pkt[129];
    uint32_t i;

    if ((mem == 0) || (len == 0U) || (len > 128U)) {
        return 0U;
    }

    pkt[0] = ctrl;
    for (i = 0U; i < len; i++) {
        pkt[i + 1U] = mem[i];
    }
    if (HAL_I2C_Master_Transmit(&hi2c1, addr8, pkt, (uint16_t)(len + 1U), 200U) != HAL_OK) {
        return 0U;
    }
    return 1U;
}

uint8_t I2C1_Probe(uint8_t addr8)
{
    if (HAL_I2C_IsDeviceReady(&hi2c1, addr8, 3U, 100U) != HAL_OK) {
        return 0U;
    }
    if (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_BUSY) != RESET) {
        (void)HAL_I2C_DeInit(&hi2c1);
        (void)HAL_I2C_Init(&hi2c1);
    }
    return 1U;
}
