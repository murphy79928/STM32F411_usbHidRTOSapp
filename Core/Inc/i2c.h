/*
 * i2c.h
 *
 *  Created on: 2026年6月23日
 *      Author: felix.chiu
 */

#ifndef INC_I2C_H_
#define INC_I2C_H_



#include "main.h"
/* Exported constants --------------------------------------------------------*/
#define I2C_EVT_TX_DONE		  (1UL << 0)
#define I2C_EVT_RX_DONE		  (1UL << 1)
#define I2C_EVT_ERROR		  (1UL << 2)

/* Wait timeout, adjust based on actual I2C speed / transfer length */
#define I2C_WAIT_TIMEOUT_MS   200


#define I2C_RECOVERY_SCL_PIN   GPIO_PIN_8   /* PB8 = I2C1_SCL */
#define I2C_RECOVERY_SDA_PIN   GPIO_PIN_7   /* PB7 = I2C1_SDA */
#define I2C_RECOVERY_PORT      GPIOB




#define I2C_RX_BUF_LEN   	 	256
#define I2C_TX_BUF_LEN    		256

extern uint8_t I2cRxBuf[I2C_RX_BUF_LEN];
extern uint16_t rxLength;
extern uint16_t rxMemAddress;
extern uint8_t i2cRW;
extern volatile uint8_t i2cTransferPending;
void I2C_Bus_Recovery(I2C_HandleTypeDef *hi2c);

#endif /* INC_I2C_H_ */
