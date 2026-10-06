/*
 * usbHID.c
 *
 *  Created on: 2026年6月23日
 *      Author: felix.chiu
 */

#include "usbHID.h"




packet_struct pcCommPkt;
const char APPInfoName[]= {'M','8','5','0','X','0','0','0'};
uint8_t UsbTxbuffer[256];
uint8_t* UsbTxbuf = &UsbTxbuffer[FIELD_DATA+1];
uint8_t gU8ReceiveData[EEPROM_TOTAL_SIZE];

static uint16_t binRecvLen  = 0;


void UsbParserByte(uint8_t data);
uint8_t UsbPCRxCmd(packet_struct* pkt);
static void CmJumpToIap(uint8_t* dat);
uint16_t GetU32(uint8_t *buf,uint32_t *data);
uint16_t GetU16(uint8_t *buf,uint16_t *data);
uint16_t PutU16(uint8_t *buf,uint16_t *data);
uint16_t PutU32(uint8_t *buf,uint32_t *data);
uint8_t CmWriteI2C(uint8_t* dat);
uint8_t CmReadI2C(uint8_t* dat);
uint8_t CmSetI2CFreq(uint8_t* dat);
uint8_t CmReadParallel(uint8_t* dat);
uint8_t CmSetIO(uint8_t *dat);
uint8_t CmWriteSPI(uint8_t *dat);
uint8_t CmReadInfo(uint8_t *dat);
uint8_t CmSendBinToMCU(uint8_t* dat);
uint8_t CmReadEEProm(uint8_t* dat);
uint8_t CmProgEEProm(uint8_t* dat);
uint8_t CmGetBinFromMCU(uint8_t* dat);
uint8_t CmI2CModeOn(uint8_t* dat);
uint8_t CmModeOff(uint8_t* dat);
uint8_t CmReadEEProm(uint8_t* dat);
uint8_t CmLEDWriteCmd(uint8_t *dat);
uint8_t CmLEDReadCmd(uint8_t *dat);
uint8_t CmFastADCReadCmd(uint8_t *dat);
typedef void (*pFunction)(void);
void PB6_SwitchToGPIO(void);
void PB6_SwitchToUART(void);

void UsbPktSendData(uint8_t CMD, uint32_t len)
{
    uint8_t ChkSum;
    uint16_t sendlen,i;
    if ((HEADER_SIZE + len) > sizeof(UsbTxbuffer)) {
        return;
    }

    UsbTxbuffer[FIELD_START]    = CMD_START;
    UsbTxbuffer[FIELD_LENGTH_L] = GET_LB(len+1);
    UsbTxbuffer[FIELD_LENGTH_H] = GET_HB(len+1);

    ChkSum = UsbTxbuffer[FIELD_START] +
             UsbTxbuffer[FIELD_LENGTH_L] +
             UsbTxbuffer[FIELD_LENGTH_H];

    UsbTxbuffer[FIELD_CHKSUM1] = GET_LB(~ChkSum)+1;

    UsbTxbuffer[FIELD_DATA] = CMD;

    ChkSum = 0;
    for (i = FIELD_DATA; i < (FIELD_DATA+1)+len; i++) {
        ChkSum += UsbTxbuffer[i];
    }

    UsbTxbuffer[FIELD_CHKSUM2+len] = GET_LB(~ChkSum)+1;

    sendlen = HEADER_SIZE + len;

    uint8_t *pBuf = pvPortMalloc(sendlen);
    if (pBuf == NULL) {
        return;
    }
    memcpy(pBuf, UsbTxbuffer, sendlen);

    UsbTxPacket_t packet = {
        .pData = pBuf,
        .length = sendlen
    };

    if (osMessageQueuePut(xUsbTxQueueHandle, &packet, 0, osWaitForever) != osOK) {
        vPortFree(pBuf);
    }
}


void UsbParserByte(uint8_t data)
{
    static uint8_t  cmdIndex = FIELD_START;
    static uint16_t Length   = 0;
    static uint16_t chkSum   = 0;

    uint16_t i;

    switch(cmdIndex)
    {
        case FIELD_START:

            if(data == CMD_START)
            {
                cmdIndex = FIELD_LENGTH_L;
            }

            break;

        case FIELD_LENGTH_L:
            Length = data;
            cmdIndex = FIELD_LENGTH_H;
            break;

        case FIELD_LENGTH_H:
            Length |= ((uint16_t)data << 8);

            if(Length > sizeof(pcCommPkt.packet))
            {
                Length = 0;
                cmdIndex = FIELD_START;
                break;
            }

            pcCommPkt.length = Length;
            cmdIndex = FIELD_CHKSUM1;
            break;

        case FIELD_CHKSUM1:

            chkSum = CMD_START
                   + GET_LB(pcCommPkt.length)
                   + GET_HB(pcCommPkt.length)
                   + data;

            if((chkSum & 0xFF) == 0)
                cmdIndex = FIELD_DATA;
            else
                cmdIndex = FIELD_START;
            break;

        case FIELD_DATA:

            pcCommPkt.packet[pcCommPkt.length - Length] = data;
            Length--;
            if(Length == 0)
                cmdIndex = FIELD_CHKSUM2;
            break;

        case FIELD_CHKSUM2:
            chkSum = data;

            for(i = 0; i < pcCommPkt.length; i++)
                chkSum += pcCommPkt.packet[i];

            if((chkSum & 0xFF) == 0)
            {
                UsbPCRxCmd(&pcCommPkt);
            }

            cmdIndex = FIELD_START;
            break;

        default:
            cmdIndex = FIELD_START;
            break;
    }
}

uint8_t UsbPCRxCmd(packet_struct* pkt)
{
	 uint8_t  cmd;
   uint8_t  *pdata;

   cmd = pkt->packet[0];
   pdata = &pkt->packet[1];
		switch( cmd )
		{
			case JUMPTOIAP:
				 CmJumpToIap(pdata);
				return 1;
			case WRITE_I2C:
				i2cRW = WRITE_I2C;
				return CmWriteI2C(pdata);
			case READ_I2C:
				i2cRW = READ_I2C;
				return CmReadI2C(pdata);
			case SET_I2CFREQ:
				return CmSetI2CFreq(pdata);
			case READ_PARA:
				return CmReadParallel(pdata);
			case SET_IO:
				return CmSetIO(pdata);
			case WRITE_SPI:
				return CmWriteSPI(pdata);
			case I2CMODEON:
				return CmI2CModeOn(pdata);
			case MODEOFF:
				return CmModeOff(pdata);
			case SENDBINTOMCU:
				return CmSendBinToMCU(pdata);
			case PROGRAMEE:
				return CmProgEEProm(pdata);
			case GETBINFROMMCU:
				return CmGetBinFromMCU(pdata);
			case READEE:
				return CmReadEEProm(pdata);
#ifdef G5031_EN
			case LEDW:
				return CmLEDWriteCmd(pdata);
			case LEDR:
				return CmLEDReadCmd(pdata);
#endif
#ifdef G8581T_EN
			case FASTADCR:
				return CmFastADCReadCmd(pdata);
#endif
			case READINFO:
				return CmReadInfo(pdata);
			default:
				return 0;
		}

}
// Switch PB6 to GPIO output
void PB6_SwitchToGPIO(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

// Switch PB6 back to UART1 TX (AF7)
void PB6_SwitchToUART(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

uint16_t PutU16(uint8_t *buf,uint16_t *data)
{
    U16F u16_val;

    u16_val.dat = *data;

    buf[0]= u16_val.bytes.b0;
    buf[1]= u16_val.bytes.b1;

    return 2;
}

uint16_t PutU32(uint8_t *buf,uint32_t *data)
{
	U32F u32_val;
	u32_val.dat = *data;

    buf[0]= u32_val.bytes.b0;
    buf[1]= u32_val.bytes.b1;
    buf[2]= u32_val.bytes.b2;
    buf[3]= u32_val.bytes.b3;

	return 4;
}


uint16_t GetU32(uint8_t *buf,uint32_t *data)
{
    U32F u32_val;

	u32_val.bytes.b0= buf[0];
	u32_val.bytes.b1= buf[1];
	u32_val.bytes.b2= buf[2];
	u32_val.bytes.b3= buf[3];

	*data = u32_val.dat;

    return 4;
}
uint16_t GetU16(uint8_t *buf,uint16_t *data)
{
    U16F u16_val;

	u16_val.bytes.b0= buf[0];
	u16_val.bytes.b1= buf[1];

	*data = u16_val.dat;

    return 2;
}

uint8_t CmWriteI2C(uint8_t* dat)
{
    uint8_t *pt = dat;
    uint8_t memAddrMode = *pt++;
    uint8_t devAddr = *pt++;
    uint8_t len = *pt++;
    uint16_t regAddr;
    HAL_StatusTypeDef status;

    i2cTransferPending = 1;  // set before starting transfer

    pt += GetU16(pt, &regAddr);

    if (memAddrMode == 0)
    {
        status = HAL_I2C_Mem_Write_DMA(
            &hi2c1,
            (devAddr << 1),
            regAddr,
            I2C_MEMADD_SIZE_8BIT,
            pt,
            len
        );
        if (status != HAL_OK)
        {
            i2cTransferPending = 0;
            xTaskNotify(myI2CEventTaskHandle, I2C_EVT_ERROR, eSetBits);
            return 0;
        }
        /* DMA path: completion handled by HAL_I2C_MemTxCpltCallback / HAL_I2C_ErrorCallback */
    }
    else
    {
        /* 16-bit blocking path: no callback will fire, notify manually */
        status = HAL_I2C_Mem_Write(
            &hi2c1,
            (devAddr << 1),
            regAddr,
            I2C_MEMADD_SIZE_16BIT,
            pt,
            len,
            100
        );
        if (status != HAL_OK)
        {
            i2cTransferPending = 0;
            xTaskNotify(myI2CEventTaskHandle, I2C_EVT_ERROR, eSetBits);
            return 0;
        }
        i2cTransferPending = 0;
        xTaskNotify(myI2CEventTaskHandle, I2C_EVT_TX_DONE, eSetBits);
    }

    return 1;
}

uint8_t CmReadI2C(uint8_t* dat)
{
    uint8_t *pt = dat;
    uint8_t memAddrMode = *pt++;
    uint8_t devAddr = *pt++;
    uint8_t len = *pt++;
    uint16_t regAddr;
    HAL_StatusTypeDef status;

    i2cTransferPending = 1;  // set before starting transfer

    pt += GetU16(pt, &regAddr);

    if (memAddrMode == 0)
    {
        status = HAL_I2C_Mem_Read_DMA(
            &hi2c1,
            (devAddr << 1),
            regAddr,
            I2C_MEMADD_SIZE_8BIT,
            I2cRxBuf,
            len
        );
        if (status != HAL_OK)
        {
            i2cTransferPending = 0;
            xTaskNotify(myI2CEventTaskHandle, I2C_EVT_ERROR, eSetBits);
            return 0;
        }
        /* DMA path: completion handled by HAL_I2C_MemRxCpltCallback / HAL_I2C_ErrorCallback */
    }
    else
    {
        /* 16-bit blocking path: no callback will fire, notify manually */
        status = HAL_I2C_Mem_Read(
            &hi2c1,
            (devAddr << 1),
            regAddr,
            I2C_MEMADD_SIZE_16BIT,
            I2cRxBuf,
            len,
            100
        );
        if (status != HAL_OK)
        {
            i2cTransferPending = 0;
            xTaskNotify(myI2CEventTaskHandle, I2C_EVT_ERROR, eSetBits);
            return 0;
        }
        i2cTransferPending = 0;

        // Manually populate what HAL_I2C_MemRxCpltCallback would have set for the DMA path
        rxLength = len;
        rxMemAddress = regAddr;
        xTaskNotify(myI2CEventTaskHandle, I2C_EVT_RX_DONE, eSetBits);
    }

    return 1;
}

uint8_t CmSetI2CFreq(uint8_t* dat)
{
	uint8_t *pt = dat;
	uint8_t status = 0;

//	pt += GetU32(pt, &u32_i2cFreq);
//
//	if(HAL_I2C_DeInit(&I2cHandle) == HAL_OK)
//	{
//		I2C_Init(u32_i2cFreq);
//		status = 1;
//	}

	UsbTxbuf[0] = status;
	UsbPktSendData(SET_I2CFREQ, 1);
	return 1;
}

uint8_t CmI2CModeOn(uint8_t* dat)
{
	uint8_t *pt = dat;
	uint16_t delayMS;
	uint16_t checkaddr;
	HAL_StatusTypeDef status;
	uint8_t dummy;
	uint8_t passflag = 0;


	pt += GetU16(pt,&delayMS);
    pt += GetU16(pt, &checkaddr);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);	//Set High Volt Mode
	EN_PWR;
	osDelay(delayMS);

    HAL_I2C_Master_Receive(
        &hi2c1,
        *pt++,
		&dummy,
        1,
        100
    );

    HAL_I2C_Master_Transmit(
        &hi2c1,
		*pt++,
        &dummy,
        1,
        100
    );
    HAL_I2C_Master_Transmit(
        &hi2c1,
		*pt++,
        &dummy,
        1,
        100
    );


    //Read I2C once to check if the device entered the mode.
	status = HAL_I2C_Mem_Read(
	&hi2c1,
	(0x24 << 1),
	checkaddr,
	I2C_MEMADD_SIZE_16BIT,
	I2cRxBuf,
	1,
	100
	);

	if (status == HAL_OK)
		passflag = 1;

	UsbTxbuf[0] = passflag;
	UsbPktSendData(I2CMODEON,1);


	return 1;
}

uint8_t CmModeOff(uint8_t* dat)
{
	DIS_PWR;
	UsbTxbuf[0] = 1;
	UsbPktSendData(MODEOFF,1);
	return 1;
}

uint8_t CmSendBinToMCU(uint8_t* dat)
{
    uint16_t offset = dat[0] | ((uint16_t)dat[1] << 8);
    uint16_t chunkLen = dat[2] | ((uint16_t)dat[3] << 8);
    uint8_t *chunkData = &dat[4];
    uint8_t *pt=UsbTxbuf;
    uint16_t len;
    uint8_t status = 1; // default
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    if (offset + chunkLen > EEPROM_TOTAL_SIZE) {
    	status= 0; // overflow, reject
    }
    else
    {
		if (offset == 0) {
			binRecvLen = 0; //Reset
		}
	    memcpy(&gU8ReceiveData[offset], chunkData, chunkLen);
	    binRecvLen += chunkLen;
    }

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

    *pt++ = status;
    *pt++ = (uint8_t)(offset & 0xFF);
    *pt++ = (uint8_t)((offset >> 8) & 0xFF);
    *pt++ = (uint8_t)(chunkLen & 0xFF);
    *pt++ = (uint8_t)((chunkLen >> 8) & 0xFF);
    len = pt-UsbTxbuf;
	UsbPktSendData(SENDBINTOMCU,len);

	return 1;
}

static uint8_t CmAccessEEProm(uint8_t* dat, EEPromOp_t op, uint8_t cmdId)
{
    HAL_StatusTypeDef status;
    uint8_t  devAddr;
    uint16_t i, StartPage, EndPage;
    uint8_t  respStatus = 1;
    uint8_t  failPage = 0xFF; // 0xFF means PASS

    devAddr   = dat[0];
    StartPage = (uint16_t)dat[1] | ((uint16_t)dat[2] << 8);
    EndPage   = (uint16_t)dat[3] | ((uint16_t)dat[4] << 8);

    // Number of EEPROM pages that fit into gU8ReceiveData (one chunk)
    const uint16_t bufPages = sizeof(gU8ReceiveData) / EEPROM_PAGE_SIZE;

    if (EndPage >= EEPROM_TOTAL_PAGES ||
        StartPage > EndPage ||
        (EndPage - StartPage + 1) > bufPages)   // Reject ranges that would overflow the buffer
    {
        respStatus = 0;
    }
    else
    {
        for (i = StartPage; i <= EndPage; i++)
        {
            uint16_t eeAddr = i * EEPROM_PAGE_SIZE;                  // Global byte address inside the EEPROM
            uint16_t bufIdx = (i - StartPage) * EEPROM_PAGE_SIZE;    // Index inside gU8ReceiveData, always starts at 0 for each chunk

            if (op == EEPROM_OP_READ)
            {
                status = HAL_I2C_Mem_Read(&hi2c1, (devAddr << 1), eeAddr,
                                          I2C_MEMADD_SIZE_16BIT,
                                          &gU8ReceiveData[bufIdx],
                                          EEPROM_PAGE_SIZE, 100);
            }
            else
            {
                status = HAL_I2C_Mem_Write(&hi2c1, (devAddr << 1), eeAddr,
                                           I2C_MEMADD_SIZE_16BIT,
                                           &gU8ReceiveData[bufIdx],
                                           EEPROM_PAGE_SIZE, 100);
                if (status == HAL_OK)
                {
                    // EEPROM internal write cycle time, only needed after write
                    vTaskDelay(pdMS_TO_TICKS(EEPROM_WRITE_DELAY_MS));
                }
            }

            if (status != HAL_OK)
            {
                respStatus = 0;
                failPage = (uint8_t)i;   // Truncated to 8 bits for pages above 255
                break;
            }
        }
    }

    UsbTxbuf[0] = respStatus;
    UsbTxbuf[1] = (uint8_t)(StartPage >> 8);
    UsbTxbuf[2] = (uint8_t)(StartPage & 0xff);
    UsbTxbuf[3] = (uint8_t)(EndPage >> 8);
    UsbTxbuf[4] = (uint8_t)(EndPage & 0xff);
    UsbTxbuf[5] = failPage;
    UsbPktSendData(cmdId, 6);

    return respStatus;
}

uint8_t CmReadEEProm(uint8_t* dat)
{
    return CmAccessEEProm(dat, EEPROM_OP_READ, READEE);
}

uint8_t CmProgEEProm(uint8_t* dat)
{
    return CmAccessEEProm(dat, EEPROM_OP_WRITE, PROGRAMEE);
}

uint8_t CmGetBinFromMCU(uint8_t* dat)
{
	uint8_t* pt = UsbTxbuf;
	uint32_t offset = (uint32_t)dat[0]
					 | ((uint32_t)dat[1] << 8)
					 | ((uint32_t)dat[2] << 16)
					 | ((uint32_t)dat[3] << 24);
	uint16_t chunkLen = BIN_CHUNK_SIZE;
    if (offset >= EEPROM_TOTAL_SIZE)
    {
        return 0; // invalid offset
    }

    if (offset + chunkLen > EEPROM_TOTAL_SIZE)
    {
        chunkLen = (uint16_t)(EEPROM_TOTAL_SIZE - offset); // last chunk shorter
    }

    pt += PutU32(pt, &offset);
    memcpy(pt, &gU8ReceiveData[offset], chunkLen);
    UsbPktSendData(GETBINFROMMCU, 4 + chunkLen);

    return 1;
}

static inline uint16_t CmReadPinData()
{
	uint32_t a = GPIOA->IDR;
	uint32_t b = GPIOB->IDR;
	/*****************************************
	* MSB									LSB
	*  11, 10,  9,  8, 7,  6, 5, 4, 3, 2, 1, 0
	* B15,B14,B13,B12,B3,A10,A9,A8,A7,A6,A5,A3
	*****************************************/
	uint16_t data =
					((b & 0xF000u) >> 4)    |   // B15..12 -> 11..8
					((b & (1u << 3)) << 4)  |   // B3      -> 7
					((a & 0x07E0u) >> 4)    |   // A10..5  -> 6..1
					((a & (1u << 3)) >> 3);     // A3      -> 0
	return data;
}

uint8_t CmReadParallel(uint8_t* dat)
{
	uint16_t len;
	uint8_t *pt=UsbTxbuf;

	uint16_t rdData = CmReadPinData();

	pt += PutU16(pt,&rdData);
	len = pt-UsbTxbuf;
	UsbPktSendData(READ_PARA,len);

	return 1;
}


uint8_t CmSetIO(uint8_t *dat)
{
    uint16_t len;
    uint8_t *pt=dat;
    uint8_t group = *pt++;
    uint8_t data = *pt++;
    uint16_t pin = *pt++ << 8;
    pin += *pt++ & 0xff;

    if(group == 0)
    {
        HAL_GPIO_WritePin(GPIOA, pin, data ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    else if(group == 1)
    {

        if(pin == GPIO_PIN_6 && PB6_CurrentMode != PB6_MODE_GPIO)
        {
            while(huart1.gState != HAL_UART_STATE_READY);
            PB6_SwitchToGPIO();
            PB6_CurrentMode = PB6_MODE_GPIO;
        }
        HAL_GPIO_WritePin(GPIOB, pin, data ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOC, pin, data ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }


    pt = UsbTxbuf;
    *pt++ = 1;
    len = pt - UsbTxbuf;
    UsbPktSendData(SET_IO, len);
    return 1;
}
uint8_t CmWriteSPI(uint8_t *dat)
{

	uint8_t *pt=dat;
	uint8_t instance=*pt++;
	uint8_t len = *pt++;
	HAL_StatusTypeDef status;


	if(instance == 0)
	{
		spi1RW=WRITE_SPI;
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

		status = HAL_SPI_Transmit_DMA(
		&hspi1,
		pt,
		len
		);
		if (status != HAL_OK)
		{
			xTaskNotify(mySPIEventTaskHandle, SPI1_EVT_ERROR, eSetBits);
			return 0;
		}
	}
	else if(instance == 1)
	{
		spi2RW=WRITE_SPI;
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

		status = HAL_SPI_Transmit_DMA(
		&hspi2,
		pt,
		len
		);
		if (status != HAL_OK)
		{
			xTaskNotify(mySPIEventTaskHandle, SPI2_EVT_ERROR, eSetBits);
			return 0;
		}
	}
	return 1;
}


uint8_t CmReadInfo(uint8_t *dat)
{
	uint8_t i;
	uint8_t *pt=UsbTxbuf;
	uint16_t len;

	for( i = 0; i < sizeof(APPInfoName) ; i++)
		 *pt++ = APPInfoName[i];

	len = pt-UsbTxbuf;
	UsbPktSendData(READINFO,len);

	return 1;
}


void HID_Bootloader_Jump(uint32_t addr)
{
    pFunction JumpToApplication;
    uint32_t JumpAddress;


    if(((*(__IO uint32_t*)addr) & 0x2FFE0000) == 0x20000000)
    {
        JumpAddress = *(__IO uint32_t*)(addr + 4);
        JumpToApplication = (pFunction)JumpAddress;

        __set_MSP(*(__IO uint32_t*)addr);

        JumpToApplication();
    }
}

static void CmJumpToIap(uint8_t* dat)
{

    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, BOOT_FLAG_VALUE);
    HAL_Delay(10);

    __disable_irq();
    NVIC_SystemReset();
    while(1);
}



#ifdef G8581T_EN

#define ADC_GPIOA_PIN_MASK ( (3UL<<(0*2)) | (3UL<<(1*2)) | (3UL<<(2*2)) | (3UL<<(3*2)) | \
                              (3UL<<(8*2)) | (3UL<<(9*2)) | (3UL<<(10*2)) | (3UL<<(15*2)) )
#define ADC_GPIOB_PIN_MASK ( (3UL<<(0*2)) | (3UL<<(1*2)) | (3UL<<(2*2)) | (3UL<<(4*2)) )

static inline uint16_t CmReadFastADCData()
{
	uint32_t a = GPIOA->IDR;
	uint32_t b = GPIOB->IDR;
	/*****************************************
	* MSB									LSB
	*  11, 10,  9,  8, 7,  6, 5, 4, 3, 2, 1, 0
	* B4, B2, B1, B0,A15,A10,A9,A8,A3,A2,A1,A0
	*****************************************/
	uint16_t data =
					(a & 0x000Fu)          |   // A3..A0  -> bit3..0 (already aligned)
					((a & 0x0700u) >> 4)   |   // A10..A8 -> bit6..4
					((a & 0x8000u) >> 8)   |   // A15     -> bit7
					((b & 0x0007u) << 8)   |   // B2..B0  -> bit10..8
					((b & 0x0010u) << 7);      // B4      -> bit11
	return data;
}



uint8_t CmFastADCReadCmd(uint8_t *dat)
{
	uint8_t *pt = dat;
	uint16_t rdData;
	uint16_t len;

	// Save current mode of the pins this function will repurpose
	uint32_t moderA_backup = GPIOA->MODER;
	uint32_t moderB_backup = GPIOB->MODER;

	// Temporarily force these pins to Input mode (00) for the digital read
	GPIOA->MODER &= ~ADC_GPIOA_PIN_MASK;
	GPIOB->MODER &= ~ADC_GPIOB_PIN_MASK;

	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_RESET);

	rdData = CmReadPinData();

	// Restore pins to whatever mode they had before this function ran
	GPIOA->MODER = moderA_backup;
	GPIOB->MODER = moderB_backup;

    pt = UsbTxbuf;
    *pt++ = 1;
	pt += PutU16(pt,&rdData);
    len = pt - UsbTxbuf;
	UsbPktSendData(FASTADCR,len);
	return 1;
}
#endif


#ifdef G5031_EN
/*******************************
 * this function is for G5031
 * PB.0=SCKI, PB.1=STDI
 * ******************************/

static inline void CmLEDBit(uint8_t bit)
{

	GPIOB->BSRR = bit ? GPIO_PIN_1 : ((uint32_t)GPIO_PIN_1 << 16); // data bit
    __NOP(); __NOP(); __NOP(); __NOP();
    GPIOB->BSRR = GPIO_PIN_0;                                      // clock high
    __NOP(); __NOP(); __NOP(); __NOP();
    GPIOB->BSRR = ((uint32_t)GPIO_PIN_0 << 16);                    // clock low
    __NOP(); __NOP(); __NOP(); __NOP();
}


static inline void CmLEDStreamOut(uint8_t *buf, uint16_t nbits, uint8_t count)
{
    taskENTER_CRITICAL();
    for(uint8_t num= 0; num < count; num++)
    {
		for (uint16_t bitpos = 0; bitpos < nbits; bitpos++)
		{
			uint8_t byteIdx   = bitpos >> 3;
			uint8_t bitInByte = 7 - (bitpos & 7);
			CmLEDBit((buf[byteIdx] >> bitInByte) & 1);
		}
    }
    taskEXIT_CRITICAL();
}

static uint16_t CmLEDReadBits(uint8_t nbits)
{
    uint16_t result = 0;

    for(uint8_t i = 0; i < nbits; i++)
    {
        GPIOB->BSRR = GPIO_PIN_0;                       // clock high
        __NOP(); __NOP(); __NOP(); __NOP();              // 跟寫入時同樣的 high 寬度

        uint8_t bit = (GPIOA->IDR & GPIO_PIN_7) ? 1 : 0;  // PA7
        result = (result << 1) | bit;

        GPIOB->BSRR = ((uint32_t)GPIO_PIN_0 << 16);      // clock low
        __NOP(); __NOP(); __NOP(); __NOP();              // low 寬度,依協定調整
    }

    return result;
}

uint8_t CmLEDReadCmd(uint8_t *dat)
{
    uint8_t *pt = dat;
    uint16_t len;
    uint16_t rdData;
    uint8_t cmd = dat[0];
    uint8_t efcode = dat[1];


    CmLEDStreamOut(dat, 224, 1);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1,  GPIO_PIN_RESET);		//End


    if((cmd == 0xA8) && (efcode == 0x00))
    {
    	//Treg
		osDelay(1);
		rdData = CmLEDReadBits(10);
    }
    else if((cmd == 0xAA) && (efcode == 0xB0))
    {
    	//eFuse
    	osDelay(1);
    	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    	//2us
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	//
    	//2us
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	//
    	//2us
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	//
    	//2us
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();
    	__NOP(); __NOP(); __NOP(); __NOP();

    	rdData = (GPIOA->IDR & GPIO_PIN_7) ? 1 : 0;  // PA7
    	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0,  GPIO_PIN_RESET);
    }



    pt = UsbTxbuf;
    *pt++ = 1;
	pt += PutU16(pt,&rdData);
    len = pt - UsbTxbuf;
    UsbPktSendData(LEDR, len);

    return 1;
}

uint8_t CmLEDWriteCmd(uint8_t *dat)
{
    uint8_t *pt = &dat[1];
    uint16_t len;
    uint8_t cnt = dat[0];
    uint8_t cmd = dat[1];
    uint8_t efcode = dat[2];


    CmLEDStreamOut(pt, 224, cnt);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1,  GPIO_PIN_RESET);			//End


    if((cmd == 0xAA) && (efcode == 0xB0))
    {
    	//eFuse
    	osDelay(1);
    	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    	osDelay(1);
    	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0,  GPIO_PIN_RESET);
    }

    pt = UsbTxbuf;
	*pt++ = 1;
	len = pt - UsbTxbuf;
    UsbPktSendData(LEDW, len);

    return 1;

}

#endif






