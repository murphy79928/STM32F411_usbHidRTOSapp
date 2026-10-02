/*
 * usbHID.h
 *
 *  Created on: 2026年6月23日
 *      Author: felix.chiu
 */

#ifndef INC_USBHID_H_
#define INC_USBHID_H_

#include "main.h"


#define USB_QUEUE_LENGTH 					128
#define USB_QUEUE_ITEM_SIZE 				CUSTOM_HID_EPOUT_SIZE
#define BUFFER_SIZE                  		512
#define GET_LB(v)                   		((v)&0xff)
#define GET_HB(v)                   		((v)>>8)
#define CMD_START                   		0xAB
#define BROADCAST                   		0x80


#define HEADER_LEN							(FIELD_CHKSUM1+1)
#define HID_BUF_LEN							CUSTOM_HID_EPIN_SIZE
#define TRAILER_LEN     					1


#define USBD_HID_IAP_DEFAULT_ADD			(FLASH_BASE)
#define BOOT_FLAG_ADDR  					0x2000FFF0
#define BOOT_FLAG_VALUE 					0x12345678


#define EEPROM_TOTAL_SIZE					16384
#define EEPROM_WRITE_DELAY_MS				5
#define EEPROM_PAGE_SIZE					32
#define EEPROM_TOTAL_PAGES  				(EEPROM_TOTAL_SIZE / EEPROM_PAGE_SIZE)  // 16384/32 = 512
#define BIN_CHUNK_SIZE       				246U   // 4-byte offset header + 247 data <= 251 max payload
enum CMD{

//	IAPWRITE = 1,
//	IAPERASE = 2,
//	JUMPTOAPP = 3,
	JUMPTOIAP = 4,
//	DATARECEIVED = 8,
	WRITE_I2C = 11,
	READ_I2C,
	SET_I2CFREQ,
	READ_PARA,
	SET_IO,
	WRITE_SPI,
	READ_SPI,

	ISDOWNLOAD = 21,
    IDPAGE,
	SENDBINTOMCU,
    PROGRAMEE,
	GETBINFROMMCU,
	READEE,

	I2CMODEON =0x83,
	MODEOFF,
	UARTMODEON,


	LEDW=0x90,
	LEDR,

	FASTADCR=0xA0,

	READINFO = 0xff
};

typedef enum
{
    EEPROM_OP_READ = 0,
    EEPROM_OP_WRITE = 1
} EEPromOp_t;

typedef union{
    uint16_t dat;
    struct{
        uint8_t b0:8;
        uint8_t b1:8;
    }bytes;
}U16F;

typedef union{
    uint32_t dat;
    struct{
        uint8_t b0:8;
        uint8_t b1:8;
			  uint8_t b2:8;
        uint8_t b3:8;
    }bytes;
}U32F;


enum USB_PACKET{
    FIELD_START=0x00,
    FIELD_LENGTH_L,
    FIELD_LENGTH_H,
    FIELD_CHKSUM1,
    FIELD_DATA,
    FIELD_CHKSUM2,

	HEADER_SIZE
};

typedef struct _packet_struct{
    uint16_t    length;
    uint8_t     command;
    uint8_t     packet[BUFFER_SIZE];
}packet_struct;

typedef struct {
    uint8_t *pData;
    uint16_t length;
} UsbTxPacket_t;

uint16_t PutU16(uint8_t *buf,uint16_t *data);
void UsbParserByte(uint8_t data);
void UsbPktSendData(uint8_t CMD, uint32_t len);
extern uint8_t* UsbTxbuf;
extern uint8_t UsbTxbuffer[256];




#endif /* INC_USBHID_H_ */
