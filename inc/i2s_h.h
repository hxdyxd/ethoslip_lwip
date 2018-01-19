#ifndef _I2S_H_H_
#define _I2S_H_H_

#include "FreeRTOS.h"
#include "au_fifo.h"
#include "i2s_api.h"
//#include "i2c_api.h"
#include "K24cxx.h"
#include "stdint.h"
#include "app_debug.h"

#define I2S_WS_PIN              PC_0
#define I2S_SCLK_PIN            PC_1
#define I2S_SD_PIN              PC_2

#define MBED_I2C_SLAVE_ADDR0    0x34
#define MBED_I2C_BUS_CLK        100000  //hz

#define I2S_DMA_PAGE_SIZE	768   // 2 ~ 4096
#define I2S_DMA_PAGE_NUM    4   // Vaild number is 2~4

void i2s_device_init();
void i2s_device_set_rate(int rate, int chls);
void i2s_device_start();
void i2s_device_stop();

typedef enum
{
	IN_PATH_OFF		= 0x00,
	MIC_LEFT_ON 	= 0x01,
	MIC_RIGHT_ON	= 0x02,
	LINE_ON			= 0x04,
	AUX_ON		    = 0x08,
	DAC_ON			= 0x10,
	ADC_ON			= 0x20
}IN_PATH_E;


typedef enum
{
	OUT_PATH_OFF	= 0x00,	
	EAR_LEFT_ON 	= 0x01,	
	EAR_RIGHT_ON	= 0x02,
	SPK_ON			= 0x04,
	OUT3_4_ON		= 0x08,
}OUT_PATH_E;

#define I2S_Standard_Phillips 0
#define I2S_Standard_MSB      1
#define I2S_Standard_LSB      2

#define VOLUME_MAX		     63		/* ¡Á?¡ä¨®¨°?¨¢? */
#define VOLUME_STEP		      1

void i2c_device_init(void);
uint8_t wm8978_Reset(void);
void wm8978_CfgAudioPath(uint16_t _InPath, uint16_t _OutPath);
void wm8978_SetOUT1Volume(uint8_t _ucVolume);
void wm8978_CfgAudioIF(uint16_t _usStandard, uint8_t _ucWordLen);


#endif
