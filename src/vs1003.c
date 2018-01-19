/*
    2017 12 31
	@sococos
*/


#include "vs1003.h"

void VS_WR_Cmd(uint8_t address, uint16_t data)
{
  	while(VS_DQ == 0);
	vs1003_hdl_spi_init(2000000, 16);
	VS_XDCS(1);
	VS_XCS(0);
	VS_SPI_ReadWriteByte( (VS_WRITE_COMMAND<<8)|address );   //MSB
	VS_SPI_ReadWriteByte(data);   //MSB
	VS_XCS(1);       
	vs1003_hdl_spi_init(8000000, 16);
}

uint16_t VS_RD_Reg(uint8_t address)
{
	uint16_t temp = 0;
    while(VS_DQ == 0);
	vs1003_hdl_spi_init(2000000, 16);
	VS_XDCS(1);
	VS_XCS(0);
	VS_SPI_ReadWriteByte( (VS_READ_COMMAND<<8)|address );   //MSB
	temp = VS_SPI_ReadWriteByte(0xffff);   //MSB
	VS_XCS(1);
	vs1003_hdl_spi_init(8000000, 16);
   	return temp;
}

uint16_t VS_WRAM_Read(uint16_t addr)
{
	uint16_t res;
 	VS_WR_Cmd(SPI_WRAMADDR, addr);
	res = VS_RD_Reg(SPI_WRAM);
 	return res;
}

void VS_Init(void)
{
  	vs1003_hdl_init();
  	vs1003_hdl_gpio_init();
  	vs1003_hdl_spi_init(2000000, 16);
}

void VS_Soft_Reset(void)
{
	uint8_t retry = 0;		   
	while(VS_DQ == 0);
	VS_SPI_ReadWriteByte(0Xff);
	retry = 0;
	while(VS_RD_Reg(SPI_MODE) != 0x0800) {
		VS_WR_Cmd(SPI_MODE, 0x0804);
		SLEEP_MS(2);
		if(retry++ > 100)
		  	break;
	}
	while(VS_DQ == 0);
	retry = 0;
	while(VS_RD_Reg(SPI_CLOCKF) != 0X9800)
	{
		VS_WR_Cmd(SPI_CLOCKF, 0X9800);
		if(retry++ > 100)
		  	break;
	}		    										    
	SLEEP_MS(20);
}

uint8_t VS_HD_Reset(void)
{
	uint8_t retry = 0;
	VS_XRST(0);
	SLEEP_MS(20);
	VS_XDCS(1);
	VS_XCS(1);
	VS_XRST(1);
	while(VS_DQ == 0 && retry < 10)
	{
		retry++;
		SLEEP_MS(1);
	};
	SLEEP_MS(20);
	if(retry >= 10)
	  	return 1;
	else
	  	return 0;	    		 
}

uint8_t VS_Send_MusicData(uint8_t *buf)
{
	uint8_t n;
	uint16_t *buf16 = (uint16_t *)buf;
	if(VS_DQ != 0)
	{
		VS_XDCS(0);
        for(n=0; n<16; n++)
		{
			VS_SPI_ReadWriteByte((buf16[n]&0xff)<<8 | (buf16[n]&0xff00)>>8);   //MSB
		}
		VS_XDCS(1);
	} else {
	  	return 1;
	}
	return 0;
}

uint16_t VS_Get_EndFillByte(void)
{
	return VS_WRAM_Read(0X1E06);
}

void VS_Restart_Play(void)
{
	uint16_t temp;
	uint16_t i;
	uint8_t n;
	uint8_t vsbuf[32];
	for(n=0; n<32; n++)
	  	vsbuf[n] = 0;
	temp = VS_RD_Reg(SPI_MODE);
	temp |= 1<<3;
	temp |= 1<<2;
	VS_WR_Cmd(SPI_MODE, temp);
	for(i=0;i<2048;)
	{
		if(VS_Send_MusicData(vsbuf) == 0)
		{
			i += 32;
   			temp = VS_RD_Reg(SPI_MODE);
 			if((temp & (1<<3)) == 0)
			  	break;
		}	
	}
	if(i < 2048)
	{
		temp = VS_Get_EndFillByte() & 0xff;
		for(n=0;n<32;n++)
		  	vsbuf[n] = temp;
		for(i=0;i<2052;)
		{
			if(VS_Send_MusicData(vsbuf)==0)
			  	i+=32;
		}   	
	} else {
		VS_Soft_Reset();
	}
	temp = VS_RD_Reg(SPI_HDAT0);
    temp += VS_RD_Reg(SPI_HDAT1);
	if(temp)					
	{
		VS_HD_Reset();   	
		VS_Soft_Reset(); 
	} 
}

void VS_Set_Vol(uint8_t volx)
{
    uint16_t volt = 0;
    volt = 254 - volx;
	volt <<= 8;
    volt += 254 - volx;
    VS_WR_Cmd(SPI_VOL, volt);
}

void VS_Set_Bass(uint8_t bfreq, uint8_t bass, uint8_t tfreq, uint8_t treble)
{
    uint16_t bass_set = 0;
    signed char temp = 0;
	if(treble == 0)
	  	temp = 0;
	else if(treble > 8)
	  	temp = treble - 8;
 	else temp = treble - 9;
	bass_set = temp&0X0F;
	bass_set <<= 4;
	bass_set += tfreq&0xf;
	bass_set <<= 4;
	bass_set += bass&0xf;
	bass_set <<= 4;
	bass_set += bfreq&0xf;
	VS_WR_Cmd(SPI_BASS, bass_set);
}

void VS_Set_Effect(uint8_t eft)
{
	uint16_t temp;
	temp = VS_RD_Reg(SPI_MODE);
	if(eft & 0X01)
	  	temp |= 1<<4;
	else
	  	temp &= ~(1<<5);
	if(eft&0X02)
	  	temp |= 1<<7;
	else
	  	temp &= ~(1<<7);					   
	VS_WR_Cmd(SPI_MODE, temp);  
}


_vs10xx_obj vsset=
{
	220,	//vol
	6,
	15,
	10,
	15,
	0,	
};
void VS_Set_All(void) 				
{
	VS_Set_Vol(vsset.mvol);
	VS_Set_Bass(vsset.bflimit, vsset.bass, vsset.tflimit, vsset.treble);
	VS_Set_Effect(vsset.effect);
}

void VS_Reset_DecodeTime(void)
{
	VS_WR_Cmd(SPI_DECODE_TIME, 0x0000);
	VS_WR_Cmd(SPI_DECODE_TIME, 0x0000);
}

void VS_Load_Patch(uint16_t *patch, uint16_t len) 
{
	uint16_t i;
	uint16_t addr, n, val; 
	for(i=0;i<len;)
	{
		addr = patch[i++];
		n    = patch[i++];
		if(n & 0x8000U)  //RLE run, replicate n samples 
		{ 
			n  &= 0x7FFF;
			val = patch[i++];
			while(n--)VS_WR_Cmd(addr, val);
		}else //copy run, copy n sample
		{
			while(n--) {
				val = patch[i++];
				VS_WR_Cmd(addr, val);
			}
		}
	}
}

/* Compressed plugin */
const u16 wav_plugin[40]= {
	0x0007, 0x0001, 0x8010, 0x0006, 0x001c, 0x3e12, 0xb817, 0x3e14, /* 0 */ 
	0xf812, 0x3e01, 0xb811, 0x0007, 0x9717, 0x0020, 0xffd2, 0x0030, /* 8 */ 
	0x11d1, 0x3111, 0x8024, 0x3704, 0xc024, 0x3b81, 0x8024, 0x3101, /* 10 */ 
	0x8024, 0x3b81, 0x8024, 0x3f04, 0xc024, 0x2808, 0x4800, 0x36f1, /* 18 */ 
	0x9811, 0x0007, 0x0001, 0x8028, 0x0006, 0x0002, 0x2a00, 0x040e,  
};

void recoder_enter_rec_mode(uint16_t agc)
{
  	VS_Set_Vol(0);
   	VS_WR_Cmd(SPI_BASS, 0x0000);
	VS_WR_Cmd(SPI_CLOCKF,  0x4430);   /* 2.0x 12.288MHz */
 	//VS_WR_Cmd(SPI_AICTRL0, 8000);
	VS_WR_Cmd(SPI_AICTRL0, 0x000c);    /* Div -> 12=8kHz 8=12kHz 6=16kHz */
	VS_WR_Cmd(SPI_AICTRL1, 0);
 	//VS_WR_Cmd(SPI_AICTRL1, agc);
	VS_WR_Cmd(SPI_MODE, 0x1804);
 	SLEEP_MS(5);
 	//VS_Load_Patch((uint16_t *)wav_plugin, 40);
}

int recoder_read_512bytes(uint8_t *pbuf)
{
  	uint16_t w;
	uint32_t idx = 0;
	do {
		w = VS_RD_Reg(SPI_HDAT1);
	}while(w < 256 || w >= 896);
	
	while(idx < 512 ) {
		w = VS_RD_Reg(SPI_HDAT0);
		pbuf[idx++] = w>>8;
		pbuf[idx++] = w&0XFF;
	}
	return idx;
}
