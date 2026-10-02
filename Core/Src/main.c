/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbd_customhid.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_tx;
DMA_HandleTypeDef hdma_i2c1_rx;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi1_tx;
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi2_tx;
DMA_HandleTypeDef hdma_spi2_rx;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;


volatile PB6_Mode_t PB6_CurrentMode = PB6_MODE_UART;
osMutexId_t PB6_MutexHandle;
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myLEDTask */
osThreadId_t myLEDTaskHandle;
const osThreadAttr_t myLEDTask_attributes = {
  .name = "myLEDTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for myI2CEventTask */
osThreadId_t myI2CEventTaskHandle;
const osThreadAttr_t myI2CEventTask_attributes = {
  .name = "myI2CEventTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for mySPIEventTask */
osThreadId_t mySPIEventTaskHandle;
const osThreadAttr_t mySPIEventTask_attributes = {
  .name = "mySPIEventTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for myUsbRxProcessT */
osThreadId_t myUsbRxProcessTHandle;
const osThreadAttr_t myUsbRxProcessT_attributes = {
  .name = "myUsbRxProcessT",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for myUsbTxProcessT */
osThreadId_t myUsbTxProcessTHandle;
const osThreadAttr_t myUsbTxProcessT_attributes = {
  .name = "myUsbTxProcessT",
  .stack_size = 1024 * 4,
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for xUsbRxQueue */
osMessageQueueId_t xUsbRxQueueHandle;
const osMessageQueueAttr_t xUsbRxQueue_attributes = {
  .name = "xUsbRxQueue"
};
/* Definitions for xUsbTxQueue */
osMessageQueueId_t xUsbTxQueueHandle;
const osMessageQueueAttr_t xUsbTxQueue_attributes = {
  .name = "xUsbTxQueue"
};
/* USER CODE BEGIN PV */
extern USBD_HandleTypeDef hUsbDeviceFS;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_RTC_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI1_Init(void);
void StartDefaultTask(void *argument);
void StartLEDTask(void *argument);
void StartI2CEventTask(void *argument);
void StartSPIEventTask(void *argument);
void StartUsbRxProcessTask(void *argument);
void StartUsbTxProcessTask(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
//	SCB->VTOR=FLASH_BASE|0x8000;	/*Shift Interrupt Vector*/
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_RTC_Init();
  MX_SPI2_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of xUsbRxQueue */
  xUsbRxQueueHandle = osMessageQueueNew (4, CUSTOM_HID_EPOUT_SIZE, &xUsbRxQueue_attributes);
  /* creation of xUsbTxQueue */
  xUsbTxQueueHandle = osMessageQueueNew (4, sizeof(UsbTxPacket_t), &xUsbTxQueue_attributes);
  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of myLEDTask */
  myLEDTaskHandle = osThreadNew(StartLEDTask, NULL, &myLEDTask_attributes);

  /* creation of myI2CEventTask */
  myI2CEventTaskHandle = osThreadNew(StartI2CEventTask, NULL, &myI2CEventTask_attributes);

  /* creation of mySPIEventTask */
  mySPIEventTaskHandle = osThreadNew(StartSPIEventTask, NULL, &mySPIEventTask_attributes);

  /* creation of myUsbRxProcessT */
  myUsbRxProcessTHandle = osThreadNew(StartUsbRxProcessTask, NULL, &myUsbRxProcessT_attributes);

  /* creation of myUsbTxProcessT */
  myUsbTxProcessTHandle = osThreadNew(StartUsbTxProcessTask, NULL, &myUsbTxProcessT_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 5;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 400000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_16_9;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}


/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_9B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_ODD;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
  /* DMA2_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
                          |GPIO_PIN_12|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pins : PC13 PC14 PC15 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA0 PA1 PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA3
                           PA8 PA9 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB1 PB2 PB10
                           PB12 PB4 PB5 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_10
                          |GPIO_PIN_12|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartLEDTask */
/**
* @brief Function implementing the myLEDTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLEDTask */
void StartLEDTask(void *argument)
{
  /* USER CODE BEGIN StartLEDTask */
  /* Infinite loop */
  for(;;)
  {
	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    osDelay(500);
  }
  /* USER CODE END StartLEDTask */
}

/* USER CODE BEGIN Header_StartI2CEventTask */
/**
* @brief Function implementing the myI2CEventTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartI2CEventTask */
void StartI2CEventTask(void *argument)
{
  /* USER CODE BEGIN StartI2CEventTask */
	uint32_t ulNotifiedValue;
	uint16_t i;
	BaseType_t notifyResult;
  /* Infinite loop */
  for(;;)
  {
	  notifyResult = xTaskNotifyWait(
	             0,
	             UINT32_MAX,
	             &ulNotifiedValue,
				 pdMS_TO_TICKS(I2C_WAIT_TIMEOUT_MS)
	         );

	  /* ------------------------------------------------------
	   * Timeout handling: the expected notification
	   * (TX_DONE/RX_DONE/ERROR) never arrived. This most likely
	   * means the I2C peripheral is stuck on the known BTF/STOP
	   * condition bug. Run bus recovery, then skip this loop
	   * iteration and go back to waiting for the next event.
	   * ------------------------------------------------------ */
	  if (notifyResult == pdFALSE)
	  {
		    if (i2cTransferPending)
		    {
				i2cTransferPending = 0;
				I2C_Bus_Recovery(&hi2c1);
				/* Optional: actively report an error so the USB host knows
				* this request failed, instead of hanging silently. If your
				* protocol already relies on the host side timing out and
				* retrying requests on its own, this block can be omitted. */
				memset(UsbTxbuffer, 0, sizeof(UsbTxbuffer));
				UsbTxbuf[0] = (uint8_t)HAL_I2C_ERROR_TIMEOUT;
				UsbPktSendData(i2cRW, 1);
		    }
		  continue;
	  }



	  	uint8_t* pt= UsbTxbuf;
		uint16_t len;
		memset(UsbTxbuffer, 0, sizeof(UsbTxbuffer));		//Clear TX Buffer
		if (ulNotifiedValue & I2C_EVT_TX_DONE) {
			if(i2cRW == WRITE_I2C)
			{
				/* Send TX OK */
				*pt++ = hi2c1.ErrorCode;
				len = pt-UsbTxbuf;
				UsbPktSendData(i2cRW,len);
			}
		}
		/* Handle RX complete event */
		if (ulNotifiedValue & I2C_EVT_RX_DONE) {
			if(i2cRW == READ_I2C)
			{
				/* Send received data back over HID */
				*pt++ = hi2c1.ErrorCode;
				pt += PutU16(pt, &rxMemAddress);  // change to 16-bit
				for(i = 0; i < rxLength; i++)
					*pt++ = I2cRxBuf[i];
				len = pt-UsbTxbuf;
				UsbPktSendData(i2cRW,len);
			}
		}

		/* Handle error event */
		if (ulNotifiedValue & I2C_EVT_ERROR) {
		  /* Report error */
			*pt++ = hi2c1.ErrorCode;
			len = pt-UsbTxbuf;
			UsbPktSendData(i2cRW,len);

			/* When DMA gets stuck / the peripheral enters an error state,
			 * the HAL's internal error handling alone may not bring the
			 * peripheral back to a clean state. Run bus recovery here as
			 * well to avoid the next transfer failing immediately due to
			 * an abnormal bus state. */
			I2C_Bus_Recovery(&hi2c1);
		}
  	  }
  /* USER CODE END StartI2CEventTask */
}

/* USER CODE BEGIN Header_StartSPIEventTask */
/**
* @brief Function implementing the mySPIEventTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSPIEventTask */
void StartSPIEventTask(void *argument)
{
  /* USER CODE BEGIN StartSPIEventTask */
	uint32_t ulNotifiedValue;
  /* Infinite loop */
  for(;;)
  {
	xTaskNotifyWait(
			0,
			UINT32_MAX,
			&ulNotifiedValue,
			portMAX_DELAY
	);
	uint8_t* pt= UsbTxbuf;
	uint16_t len;
	if (ulNotifiedValue & SPI1_EVT_TX_DONE)
	{
		*pt++ = hspi1.ErrorCode;
		len = pt-UsbTxbuf;
		UsbPktSendData(spi1RW,len);
	}
	if(ulNotifiedValue & SPI1_EVT_ERROR)
	{
		*pt++ = hspi1.ErrorCode;
		len = pt-UsbTxbuf;
		UsbPktSendData(spi1RW,len);
	}
	if (ulNotifiedValue & SPI2_EVT_TX_DONE)
	{
		*pt++ = hspi2.ErrorCode;
		len = pt-UsbTxbuf;
		UsbPktSendData(spi2RW,len);
	}
	if(ulNotifiedValue & SPI2_EVT_ERROR)
	{
		*pt++ = hspi2.ErrorCode;
		len = pt-UsbTxbuf;
		UsbPktSendData(spi2RW,len);
	}
  }
  /* USER CODE END StartSPIEventTask */
}

/* USER CODE BEGIN Header_StartUsbRxProcessTask */
/**
* @brief Function implementing the myUsbRxProcessT thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUsbRxProcessTask */
void StartUsbRxProcessTask(void *argument)
{
  /* USER CODE BEGIN StartUsbRxProcessTask */
	uint8_t rxData[CUSTOM_HID_EPOUT_SIZE];
  /* Infinite loop */
  for(;;)
  {
	if (osMessageQueueGet(xUsbRxQueueHandle, rxData, NULL, osWaitForever) == osOK)
	{
		for(uint8_t i = 0; i < CUSTOM_HID_EPOUT_SIZE; i++)
		{
			UsbParserByte(rxData[i]);
		}
	}
  }
  /* USER CODE END StartUsbRxProcessTask */
}

/* USER CODE BEGIN Header_StartUsbTxProcessTask */
/**
* @brief Function implementing the myUsbTxProcessT thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUsbTxProcessTask */
void StartUsbTxProcessTask(void *argument)
{
  /* USER CODE BEGIN StartUsbTxProcessTask */
  UsbTxPacket_t packet;
  uint8_t report[64];
  /* Infinite loop */
  for(;;)
  {
//	if (osMessageQueueGet(xUsbTxQueueHandle, &packet, NULL, osWaitForever) == osOK)
//	{
//		memset(report, 0, sizeof(report));
//		uint16_t len = packet.length > sizeof(report) ? sizeof(report) : packet.length;
//		memcpy(report, packet.pData, len);
//		USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
//		vPortFree(packet.pData);
//	}

    if (osMessageQueueGet(xUsbTxQueueHandle, &packet, NULL, osWaitForever) == osOK)
    {
      uint32_t sent = 0;
      while (sent < packet.length)
      {
          uint16_t chunkLen = (packet.length - sent) > sizeof(report)
                               ? sizeof(report)
                               : (uint16_t)(packet.length - sent);

          memset(report, 0, sizeof(report));
          memcpy(report, packet.pData + sent, chunkLen);


          USBD_CUSTOM_HID_HandleTypeDef *hhid = (USBD_CUSTOM_HID_HandleTypeDef*)hUsbDeviceFS.pClassData;
          while (hhid->state == CUSTOM_HID_BUSY)
          {
              vTaskDelay(1);
          }

          USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, report, sizeof(report));
          sent += chunkLen;
      }
      vPortFree(packet.pData);
    }
  }

  /* USER CODE END StartUsbTxProcessTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
