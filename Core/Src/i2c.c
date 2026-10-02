/*
 * i2c.c
 *
 *  Created on: 2026年6月23日
 *      Author: felix.chiu
 */


#include "i2c.h"


/* Handle for the I2C event processing task */
/* Buffers and length tracker */
uint8_t I2cRxBuf[I2C_RX_BUF_LEN];
uint16_t  rxLength = 0;
uint16_t  rxMemAddress = 0;
uint8_t i2cRW;

/* Add this flag, set it before starting any I2C transfer,
 * clear it when TX_DONE/RX_DONE/ERROR is received */
volatile uint8_t i2cTransferPending = 0;
/**
  * @brief  Memory Tx Transfer completed callback.
  * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @retval None
  */
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (hi2c->Instance == I2C1) {
    	i2cTransferPending = 0;
        /* Notify the task that transmission is done */
        xTaskNotifyFromISR(myI2CEventTaskHandle, I2C_EVT_TX_DONE, eSetBits, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

}

/**
  * @brief  Memory Rx Transfer completed callback.
  * @param  hi2c Pointer to a I2C_HandleTypeDef structure that contains
  *                the configuration information for the specified I2C.
  * @retval None
  */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (hi2c->Instance == I2C1) {
    	i2cTransferPending = 0;
		rxLength = hi2c->XferSize;
		rxMemAddress = hi2c->Memaddress;
        /* Store actual received length and notify the task */
        xTaskNotifyFromISR(myI2CEventTaskHandle, I2C_EVT_RX_DONE, eSetBits, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (hi2c->Instance == I2C1) {
    	i2cTransferPending = 0;
        /* Notify the task that reception is done */
        xTaskNotifyFromISR(myI2CEventTaskHandle, I2C_EVT_ERROR, eSetBits, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    }
}
/* ============================================================
 * I2C Bus Recovery
 *
 * When the I2C peripheral gets stuck (SCL held low by clock
 * stretching from the slave, or the STOP condition is never
 * generated because the BTF interrupt was not handled), a plain
 * HAL_I2C_DeInit/Init is usually not enough to recover, because
 * the slave device may still believe it's in the middle of a
 * transfer and keep driving SCL or SDA.
 *
 * Standard approach: switch SCL/SDA to GPIO mode, manually pulse
 * the clock up to 9 times to flush out any residual bit stuck in
 * the slave's internal state machine, then manually generate a
 * STOP condition. Once SDA is confirmed released, switch the pins
 * back to the I2C peripheral's alternate function and re-init.
 *
 * Note: PB7 = I2C1_SDA, PB8 = I2C1_SCL, matching the current
 * CubeMX pin mapping.
 * ============================================================ */
void I2C_Bus_Recovery(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 1. Disable the I2C peripheral first, releasing control of SCL/SDA */
    HAL_I2C_DeInit(hi2c);

    /* 2. Switch SCL/SDA to open-drain GPIO output to simulate manual bit-banging */
    GPIO_InitStruct.Pin   = I2C_RECOVERY_SCL_PIN | I2C_RECOVERY_SDA_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(I2C_RECOVERY_PORT, &GPIO_InitStruct);

    /* Initial state: release both SCL/SDA high (open-drain, writing 1 = released) */
    HAL_GPIO_WritePin(I2C_RECOVERY_PORT, I2C_RECOVERY_SCL_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(I2C_RECOVERY_PORT, I2C_RECOVERY_SDA_PIN, GPIO_PIN_SET);
    HAL_Delay(1);

    /* 3. If SDA is held low by the slave (indicating it's stuck mid-transfer),
     *    manually generate up to 9 SCL clock pulses to flush out the
     *    residual bit stuck inside the slave */
    for (int i = 0; i < 9; i++)
    {
        if (HAL_GPIO_ReadPin(I2C_RECOVERY_PORT, I2C_RECOVERY_SDA_PIN) == GPIO_PIN_SET)
        {
            /* SDA already released, slave has finished flushing, exit early */
            break;
        }

        HAL_GPIO_WritePin(I2C_RECOVERY_PORT, I2C_RECOVERY_SCL_PIN, GPIO_PIN_RESET); /* SCL low */
        HAL_Delay(1);
        HAL_GPIO_WritePin(I2C_RECOVERY_PORT, I2C_RECOVERY_SCL_PIN, GPIO_PIN_SET);   /* SCL high */
        HAL_Delay(1);
    }

    /* 4. Manually generate a STOP condition: SDA rises from low to high while SCL is high */
    HAL_GPIO_WritePin(I2C_RECOVERY_PORT, I2C_RECOVERY_SDA_PIN, GPIO_PIN_RESET); /* SDA low */
    HAL_Delay(1);
    HAL_GPIO_WritePin(I2C_RECOVERY_PORT, I2C_RECOVERY_SCL_PIN, GPIO_PIN_SET);   /* SCL high */
    HAL_Delay(1);
    HAL_GPIO_WritePin(I2C_RECOVERY_PORT, I2C_RECOVERY_SDA_PIN, GPIO_PIN_SET);   /* SDA high (STOP) */
    HAL_Delay(1);

    /* 5. Re-initialize the I2C peripheral.
     *    HAL_I2C_Init internally calls HAL_I2C_MspInit, which (generated
     *    by CubeMX) already reconfigures PB7/PB8 back to AF_I2C1 mode.
     *    No need to manually set the AF here again -- doing so risks
     *    duplicating or conflicting with CubeMX's configuration
     *    (e.g. Pull, Speed settings might not match). */
    HAL_I2C_Init(hi2c);
}



