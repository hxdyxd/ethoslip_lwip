/*
    2017 12 31
	@sococos
*/


#ifndef _VS1003_H_
#define _VS1003_H_


#include "vs1003_spi.h"


__packed typedef struct 
{							  
	u8 mvol;
	u8 bflimit;
	u8 bass;
	u8 tflimit;
	u8 treble;
	u8 effect;

	u8 saveflag;
}_vs10xx_obj;


extern _vs10xx_obj vsset;

#define VS_WRITE_COMMAND 	0x02
#define VS_READ_COMMAND 	0x03
//vs10xx reg
#define SPI_MODE        	0x00   
#define SPI_STATUS      	0x01   
#define SPI_BASS        	0x02   
#define SPI_CLOCKF      	0x03   
#define SPI_DECODE_TIME 	0x04   
#define SPI_AUDATA      	0x05   
#define SPI_WRAM        	0x06   
#define SPI_WRAMADDR    	0x07   
#define SPI_HDAT0       	0x08   
#define SPI_HDAT1       	0x09 
  
#define SPI_AIADDR      	0x0a   
#define SPI_VOL         	0x0b   
#define SPI_AICTRL0     	0x0c   
#define SPI_AICTRL1     	0x0d   
#define SPI_AICTRL2     	0x0e   
#define SPI_AICTRL3     	0x0f   
#define SM_DIFF         	0x01   
#define SM_JUMP         	0x02   
#define SM_RESET        	0x04   
#define SM_OUTOFWAV     	0x08   
#define SM_PDOWN        	0x10   
#define SM_TESTS        	0x20   
#define SM_STREAM       	0x40   
#define SM_PLUSV        	0x80   
#define SM_DACT         	0x100   
#define SM_SDIORD       	0x200   
#define SM_SDISHARE     	0x400   
#define SM_SDINEW       	0x800   
#define SM_ADPCM        	0x1000   
#define SM_ADPCM_HP     	0x2000 		 

uint16_t VS_RD_Reg(uint8_t address);
void VS_Init(void);
void VS_Soft_Reset(void);
uint8_t VS_HD_Reset(void);
uint8_t VS_Send_MusicData(uint8_t *buf);
void VS_Restart_Play(void);
void VS_Set_All(void);
void VS_Reset_DecodeTime(void);
int recoder_read_512bytes(uint8_t *pbuf);

#endif
