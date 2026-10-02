/*
 * spi.c
 *
 *  Created on: 2026年6月23日
 *      Author: felix.chiu
 */


#include "spi.h"

uint8_t spi1RW;
uint8_t spi2RW;

/**
  * @brief  TxRx Transfer completed callback.
  * @param  hspi: SPI handle.
  * @note   This example shows a simple way to report end of DMA TxRx transfer, and
  *         you can add your own implementation.
  * @retval None
  */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(hspi->Instance == SPI1)
	{
		/*SS H*/
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
		xTaskNotifyFromISR(mySPIEventTaskHandle, SPI1_EVT_TX_DONE, eSetBits, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	else if(hspi->Instance == SPI2)
	{
		/*SS H*/
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
		xTaskNotifyFromISR(mySPIEventTaskHandle, SPI2_EVT_TX_DONE, eSetBits, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

/**
  * @brief  SPI error callbacks.
  * @param  hspi: SPI handle
  * @note   This example shows a simple way to report transfer error, and you can
  *         add your own implementation.
  * @retval None
  */
 void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	if(hspi->Instance == SPI1)
	{
		/*SS H*/
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
		xTaskNotifyFromISR(mySPIEventTaskHandle, SPI1_EVT_ERROR, eSetBits, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	else if(hspi->Instance == SPI2)
	{
		/*SS H*/
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
		xTaskNotifyFromISR(mySPIEventTaskHandle, SPI2_EVT_ERROR, eSetBits, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}


