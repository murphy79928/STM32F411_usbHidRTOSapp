/*
 * spi.h
 *
 *  Created on: 2026年6月23日
 *      Author: felix.chiu
 */

#ifndef INC_SPI_H_
#define INC_SPI_H_

#include "main.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

#define SPI1_EVT_TX_DONE		  (1UL << 0)
#define SPI1_EVT_RX_DONE		  (1UL << 1)
#define SPI1_EVT_ERROR		  	  (1UL << 2)
#define SPI2_EVT_TX_DONE		  (1UL << 3)
#define SPI2_EVT_RX_DONE		  (1UL << 4)
#define SPI2_EVT_ERROR		  	  (1UL << 5)

extern uint8_t spi1RW;
extern uint8_t spi2RW;
#endif /* INC_SPI_H_ */
