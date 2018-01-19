#include "i2s_h.h"

i2s_t i2s_obj;

static uint8_t i2s_tx_buf[I2S_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];
//uint8_t i2s_rx_buf[I2S_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];
u32 count = 0;

void mi2s_tx_complete(void *data, char *pbuf)
{
	i2s_t *obj = (i2s_t *)data;
	int *ptx_buf = i2s_get_tx_page(obj);
	extern struct au_fifo_t *audio_out_fifo;
	if(audio_out_fifo != NULL) {
		int r_len = au_fifo_read(audio_out_fifo, ptx_buf, I2S_DMA_PAGE_SIZE);
		if(r_len < 0) {
	  		memset(ptx_buf, 0, I2S_DMA_PAGE_SIZE);
	  		//printf( "i2s_underrun\n");
		}
	} else {
		memset(ptx_buf, 0, I2S_DMA_PAGE_SIZE);
	}
	
	i2s_send_page(obj, (uint32_t*)ptx_buf);
}

void i2s_device_init()
{
  	i2s_obj.channel_num = CH_STEREO;
	i2s_obj.sampling_rate = SR_8KHZ;
	i2s_obj.word_length = WL_16b;
	i2s_obj.direction = I2S_DIR_TX;
  	i2s_init(&i2s_obj, I2S_SCLK_PIN, I2S_WS_PIN, I2S_SD_PIN);
	i2s_set_dma_buffer(&i2s_obj, (char*)i2s_tx_buf, (char*)NULL, \
        I2S_DMA_PAGE_NUM, I2S_DMA_PAGE_SIZE);
    i2s_tx_irq_handler(&i2s_obj, (i2s_irq_handler)mi2s_tx_complete, (uint32_t)&i2s_obj);
	
	int *ptx_buf = i2s_get_tx_page(&i2s_obj);
	i2s_send_page(&i2s_obj, (uint32_t*)ptx_buf);
}

void i2s_device_start()
{
	i2s_enable(&i2s_obj);
	printf( "i2s enable\n");
}

void i2s_device_stop()
{
	i2s_disable(&i2s_obj);
	printf( "i2s disable\n");
}

void i2s_device_set_rate(int rate, int chls)
{
  	int channel_num = (chls==1)?CH_MONO:CH_STEREO;
	int rate_t = 0;
	
	switch(rate) {
	case 8000:
	  	rate_t = SR_8KHZ;
		break;
	case 16000:
	  	rate_t = SR_16KHZ;
	  	break;
	case 24000:
	  	rate_t = SR_24KHZ;
	  	break;
	case 32000:
	  	rate_t = SR_32KHZ;
	  	break;
	case 48000:
	  	rate_t = SR_48KHZ;
	  	break;
	case 22050:
	  	rate_t = SR_22p05KHZ;
	  	break;
	case 44100:
	  	rate_t = SR_44p1KHZ;
	  	break;
	default:
	  	printf( "unknow sample rate_t\n");
		return;
	}
	
	i2s_set_param(&i2s_obj, CH_STEREO, rate_t, WL_16b);
}

i2c_t   i2cmaster;

/*
	wm8978寄存器缓存
	由于WM8978的I2C两线接口不支持读取操作，因此寄存器值缓存在内存中，
	当写寄存器时同步更新缓存，读寄存器时直接返回缓存中的值。
	寄存器MAP 在WM8978(V4.5_2011).pdf 的第89页，寄存器地址是7bit， 寄存器数据是9bit
*/
static uint16_t wm8978_RegCash[] = {
	0x000, 0x000, 0x000, 0x000, 0x050, 0x000, 0x140, 0x000,
	0x000, 0x000, 0x000, 0x0FF, 0x0FF, 0x000, 0x100, 0x0FF,
	0x0FF, 0x000, 0x12C, 0x02C, 0x02C, 0x02C, 0x02C, 0x000,
	0x032, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x038, 0x00B, 0x032, 0x000, 0x008, 0x00C, 0x093, 0x0E9,
	0x000, 0x000, 0x000, 0x000, 0x003, 0x010, 0x010, 0x100,
	0x100, 0x002, 0x001, 0x001, 0x039, 0x039, 0x039, 0x039,
	0x001, 0x001
};

/* wm8978寄存器缺省值 */
const uint16_t reg_default[] = {
	0x000, 0x000, 0x000, 0x000, 0x050, 0x000, 0x140, 0x000,
	0x000, 0x000, 0x000, 0x0FF, 0x0FF, 0x000, 0x100, 0x0FF,
	0x0FF, 0x000, 0x12C, 0x02C, 0x02C, 0x02C, 0x02C, 0x000,
	0x032, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
	0x038, 0x00B, 0x032, 0x000, 0x008, 0x00C, 0x093, 0x0E9,
	0x000, 0x000, 0x000, 0x000, 0x003, 0x010, 0x010, 0x100,
	0x100, 0x002, 0x001, 0x001, 0x039, 0x039, 0x039, 0x039,
	0x001, 0x001
};

void i2c_device_init(void)
{
  	i2c_init(&i2cmaster, MBED_I2C_MTR_SDA ,MBED_I2C_MTR_SCL);
	i2c_frequency(&i2cmaster, MBED_I2C_BUS_CLK);
}

static uint8_t wm8978_WriteReg(uint8_t _ucRegAddr, uint16_t _usValue)
{
	uint8_t res;
	uint8_t dat[2];
	dat[0] = ((_ucRegAddr << 1) & 0xFE) | ((_usValue >> 8) & 0x1);
	dat[1] =  _usValue&0xff;
	//APP_DEBUG("%d %d enter\n", _ucRegAddr, _usValue);
	/*__set_PRIMASK(1);
	IIC_Start();
    res = IIC_Send_Byte(MBED_I2C_SLAVE_ADDR0);
	res = IIC_Send_Byte(dat[0]);
	res = IIC_Send_Byte(dat[1]);
	IIC_Stop();
	__set_PRIMASK(0);
	*/
	res = i2c_write(&i2cmaster, MBED_I2C_SLAVE_ADDR0, dat, 2, 0);
	if(res == FALSE) {
	  	APP_ERROR("ACK ERROR %d %d\n", _ucRegAddr, _usValue);
	  	return 0;
	} else {
	  	int i, pt=0;
		char val[10 + 3];
		for(i=8;i>=0;i--) {
		  	val[pt++] = (_usValue & (1 << i))?'1':'0';
			if(i==8 || i==4) {
			  	val[pt++] = ' ';
			}
		}
		val[pt] = 0;
	  	APP_DEBUG("Send %d, %d \033[40;34m%s\033[0m\n", _ucRegAddr, _usValue, val);
	}
	//APP_DEBUG("%d %d end\n", _ucRegAddr, _usValue);
	wm8978_RegCash[_ucRegAddr] = _usValue;
	return 1;
}

/**
	* @brief  复位wm8978，所有的寄存器值恢复到缺省值
	* @param  无
	* @retval 1: 复位成功
	* 		  0：复位失败
	*/
uint8_t wm8978_Reset(void)
{
	uint8_t res;
	uint8_t i;

	res = wm8978_WriteReg(0x00, 0);

	for (i = 0; i < sizeof(reg_default) / 2; i++)
	{
		wm8978_RegCash[i] = reg_default[i];
	}
	return res;
}

/**
	* @brief  配置wm8978音频通道
	* @param  _InPath : 音频输入通道配置
	* @param  _OutPath : 音频输出通道配置
	* @retval 无
	*/
void wm8978_CfgAudioPath(uint16_t _InPath, uint16_t _OutPath)
{
	uint16_t usReg;

	/* 查看WM8978数据手册的 REGISTER MAP 章节， 第89页 */

	if ((_InPath == IN_PATH_OFF) && (_OutPath == OUT_PATH_OFF))
	{
		wm8978_Reset();
		return;
	}

	/*
		R1 寄存器 Power manage 1
		Bit8    BUFDCOPEN,  Output stage 1.5xAVDD/2 driver enable
 		Bit7    OUT4MIXEN, OUT4 mixer enable
		Bit6    OUT3MIXEN, OUT3 mixer enable
		Bit5    PLLEN	.不用
		Bit4    MICBEN	,Microphone Bias Enable (MIC偏置电路使能)
		Bit3    BIASEN	,Analogue amplifier bias control 必须设置为1模拟放大器才工作
		Bit2    BUFIOEN , Unused input/output tie off buffer enable
		Bit1:0  VMIDSEL, 必须设置为非00值模拟放大器才工作
	*/
	usReg = (1 << 3) | (3 << 0);
	if (_OutPath & OUT3_4_ON) 	/* OUT3和OUT4使能输出到GSM模块 */
	{
		usReg |= ((1 << 7) | (1 << 6));
	}
	if ((_InPath & MIC_LEFT_ON) || (_InPath & MIC_RIGHT_ON))
	{
		usReg |= (1 << 4);
	}
	wm8978_WriteReg(1, usReg);	/* 写寄存器 */

	/*
		R2 寄存器 Power manage 2
		Bit8	ROUT1EN,	ROUT1 output enable 耳机右声道输出使能
		Bit7	LOUT1EN,	LOUT1 output enable 耳机左声道输出使能
		Bit6	SLEEP, 		0 = Normal device operation   1 = Residual current reduced in device standby mode
		Bit5	BOOSTENR,	Right channel Input BOOST enable 输入通道自举电路使能. 用到PGA放大功能时必须使能
		Bit4	BOOSTENL,	Left channel Input BOOST enable
		Bit3	INPGAENR,	Right channel input PGA enable 右声道输入PGA使能
		Bit2	INPGAENL,	Left channel input PGA enable
		Bit1	ADCENR,		Enable ADC right channel
		Bit0	ADCENL,		Enable ADC left channel
	*/
	usReg = 0;
	if (_OutPath & EAR_LEFT_ON)
	{
		usReg |= (1 << 7);
	}
	if (_OutPath & EAR_RIGHT_ON)
	{
		usReg |= (1 << 8);
	}
	if (_InPath & MIC_LEFT_ON)
	{
		usReg |= ((1 << 4) | (1 << 2));
	}
	if (_InPath & MIC_RIGHT_ON)
	{
		usReg |= ((1 << 5) | (1 << 3));
	}
	if (_InPath & LINE_ON)
	{
		usReg |= ((1 << 4) | (1 << 5));
	}
	if (_InPath & MIC_RIGHT_ON)
	{
		usReg |= ((1 << 5) | (1 << 3));
	}
	if (_InPath & ADC_ON)
	{
		usReg |= ((1 << 1) | (1 << 0));
	}
	wm8978_WriteReg(2, usReg);	/* 写寄存器 */

	/*
		R3 寄存器 Power manage 3
		Bit8	OUT4EN,		OUT4 enable
		Bit7	OUT3EN,		OUT3 enable
		Bit6	LOUT2EN,	LOUT2 output enable
		Bit5	ROUT2EN,	ROUT2 output enable
		Bit4	0,
		Bit3	RMIXEN,		Right mixer enable
		Bit2	LMIXEN,		Left mixer enable
		Bit1	DACENR,		Right channel DAC enable
		Bit0	DACENL,		Left channel DAC enable
	*/
	usReg = 0;
	if (_OutPath & OUT3_4_ON)
	{
		usReg |= ((1 << 8) | (1 << 7));
	}
	if (_OutPath & SPK_ON)
	{
		usReg |= ((1 << 6) | (1 << 5));
	}
	if (_OutPath != OUT_PATH_OFF)
	{
		usReg |= ((1 << 3) | (1 << 2));
	}
	if (_InPath & DAC_ON)
	{
		usReg |= ((1 << 1) | (1 << 0));
	}
	wm8978_WriteReg(3, usReg);	/* 写寄存器 */

	/*
		R44 寄存器 Input ctrl

		Bit8	MBVSEL, 		Microphone Bias Voltage Control   0 = 0.9 * AVDD   1 = 0.6 * AVDD
		Bit7	0
		Bit6	R2_2INPPGA,		Connect R2 pin to right channel input PGA positive terminal
		Bit5	RIN2INPPGA,		Connect RIN pin to right channel input PGA negative terminal
		Bit4	RIP2INPPGA,		Connect RIP pin to right channel input PGA amplifier positive terminal
		Bit3	0
		Bit2	L2_2INPPGA,		Connect L2 pin to left channel input PGA positive terminal
		Bit1	LIN2INPPGA,		Connect LIN pin to left channel input PGA negative terminal
		Bit0	LIP2INPPGA,		Connect LIP pin to left channel input PGA amplifier positive terminal
	*/
	usReg = 0 << 8;
	if (_InPath & LINE_ON)
	{
		usReg |= ((1 << 6) | (1 << 2));
	}
	if (_InPath & MIC_RIGHT_ON)
	{
		usReg |= ((1 << 5) | (1 << 4));
	}
	if (_InPath & MIC_LEFT_ON)
	{
		usReg |= ((1 << 1) | (1 << 0));
	}
	wm8978_WriteReg(44, usReg);	/* 写寄存器 */


	/*
		R14 寄存器 ADC Control
		设置高通滤波器（可选的） WM8978(V4.5_2011).pdf 31 32页,
		Bit8 	HPFEN,	High Pass Filter Enable高通滤波器使能，0表示禁止，1表示使能
		BIt7 	HPFAPP,	Select audio mode or application mode 选择音频模式或应用模式，0表示音频模式，
		Bit6:4	HPFCUT，Application mode cut-off frequency  000-111选择应用模式的截止频率
		Bit3 	ADCOSR,	ADC oversample rate select: 0=64x (lower power) 1=128x (best performance)
		Bit2   	0
		Bit1 	ADC right channel polarity adjust:  0=normal  1=inverted
		Bit0 	ADC left channel polarity adjust:  0=normal 1=inverted
	*/
	if (_InPath & ADC_ON)
	{
		usReg = (1 << 3) | (0 << 8) | (4 << 0);		/* 禁止ADC高通滤波器, 设置截至频率 */
	}
	else
	{
		usReg = 0;
	}
	wm8978_WriteReg(14, usReg);	/* 写寄存器 */

	/* 设置陷波滤波器（notch filter），主要用于抑制话筒声波正反馈，避免啸叫.  暂时关闭
		R27，R28，R29，R30 用于控制限波滤波器，WM8978(V4.5_2011).pdf 33页
		R7的 Bit7 NFEN = 0 表示禁止，1表示使能
	*/
	if (_InPath & ADC_ON)
	{
		usReg = (0 << 7);
		wm8978_WriteReg(27, usReg);	/* 写寄存器 */
		usReg = 0;
		wm8978_WriteReg(28, usReg);	/* 写寄存器,填0，因为已经禁止，所以也可不做 */
		wm8978_WriteReg(29, usReg);	/* 写寄存器,填0，因为已经禁止，所以也可不做 */
		wm8978_WriteReg(30, usReg);	/* 写寄存器,填0，因为已经禁止，所以也可不做 */
	}

	/* 自动增益控制 ALC, R32  - 34  WM8978(V4.5_2011).pdf 36页 */
	{
		usReg = 0;		/* 禁止自动增益控制 */
		wm8978_WriteReg(32, usReg);
		wm8978_WriteReg(33, usReg);
		wm8978_WriteReg(34, usReg);
	}

	/*  R35  ALC Noise Gate Control
		Bit3	NGATEN, Noise gate function enable
		Bit2:0	Noise gate threshold:
	*/
	usReg = (3 << 1) | (7 << 0);		/* 禁止自动增益控制 */
	wm8978_WriteReg(35, usReg);

	/*
		Mic 输入信道的增益由 PGABOOSTL 和 PGABOOSTR 控制
		Aux 输入信道的输入增益由 AUXL2BOOSTVO[2:0] 和 AUXR2BOOSTVO[2:0] 控制
		Line 输入信道的增益由 LIP2BOOSTVOL[2:0] 和 RIP2BOOSTVOL[2:0] 控制
	*/
	/*	WM8978(V4.5_2011).pdf 29页，R47（左声道），R48（右声道）, MIC 增益控制寄存器
		R47 (R48定义与此相同)
		B8		PGABOOSTL	= 1,   0表示MIC信号直通无增益，1表示MIC信号+20dB增益（通过自举电路）
		B7		= 0， 保留
		B6:4	L2_2BOOSTVOL = x， 0表示禁止，1-7表示增益-12dB ~ +6dB  （可以衰减也可以放大）
		B3		= 0， 保留
		B2:0`	AUXL2BOOSTVOL = x，0表示禁止，1-7表示增益-12dB ~ +6dB  （可以衰减也可以放大）
	*/
	usReg = 0;
	if ((_InPath & MIC_LEFT_ON) || (_InPath & MIC_RIGHT_ON))
	{
		usReg |= (1 << 8);	/* MIC增益取+20dB */
	}
	if (_InPath & AUX_ON)
	{
		usReg |= (3 << 0);	/* Aux增益固定取3，用户可以自行调整 */
	}
	if (_InPath & LINE_ON)
	{
		usReg |= (3 << 4);	/* Line增益固定取3，用户可以自行调整 */
	}
	wm8978_WriteReg(47, usReg);	/* 写左声道输入增益控制寄存器 */
	wm8978_WriteReg(48, usReg);	/* 写右声道输入增益控制寄存器 */

	/* 数字ADC音量控制，pdf 35页
		R15 控制左声道ADC音量，R16控制右声道ADC音量
		Bit8 	ADCVU  = 1 时才更新，用于同步更新左右声道的ADC音量
		Bit7:0 	增益选择； 0000 0000 = 静音
						   0000 0001 = -127dB
						   0000 0010 = -12.5dB  （0.5dB 步长）
						   1111 1111 = 0dB  （不衰减）
	*/
	usReg = 0xFF;
	wm8978_WriteReg(15, usReg);	/* 选择0dB，先缓存左声道 */
	usReg = 0x1FF;
	wm8978_WriteReg(16, usReg);	/* 同步更新左右声道 */

	/* 通过 wm8978_SetMicGain 函数设置mic PGA增益 */

	/*	R43 寄存器  AUXR – ROUT2 BEEP Mixer Function
		B8:6 = 0

		B5	 MUTERPGA2INV,	Mute input to INVROUT2 mixer
		B4	 INVROUT2,  Invert ROUT2 output 用于扬声器推挽输出
		B3:1 BEEPVOL = 7;	AUXR input to ROUT2 inverter gain
		B0	 BEEPEN = 1;	Enable AUXR beep input

	*/
	usReg = 0;
	if (_OutPath & SPK_ON)
	{
		usReg |= (1 << 4);	/* ROUT2 反相, 用于驱动扬声器 */
	}
	if (_InPath & AUX_ON)
	{
		usReg |= ((7 << 1) | (1 << 0));
	}
	wm8978_WriteReg(43, usReg);

	/* R49  Output ctrl
		B8:7	0
		B6		DACL2RMIX,	Left DAC output to right output mixer
		B5		DACR2LMIX,	Right DAC output to left output
		B4		OUT4BOOST,	0 = OUT4 output gain = -1; DC = AVDD / 2；1 = OUT4 output gain = +1.5；DC = 1.5 x AVDD / 2
		B3		OUT3BOOST,	0 = OUT3 output gain = -1; DC = AVDD / 2；1 = OUT3 output gain = +1.5；DC = 1.5 x AVDD / 2
		B2		SPKBOOST,	0 = Speaker gain = -1;  DC = AVDD / 2 ; 1 = Speaker gain = +1.5; DC = 1.5 x AVDD / 2
		B1		TSDEN,   Thermal Shutdown Enable  扬声器热保护使能（缺省1）
		B0		VROI,	Disabled Outputs to VREF Resistance
	*/
	usReg = 0;
	if (_InPath & DAC_ON)
	{
		usReg |= ((1 << 6) | (1 << 5));
	}
	if (_OutPath & SPK_ON)
	{
		usReg |=  ((1 << 2) | (1 << 1));	/* SPK 1.5x增益,  热保护使能 */
	}
	if (_OutPath & OUT3_4_ON)
	{
		usReg |=  ((1 << 4) | (1 << 3));	/* BOOT3  BOOT4  1.5x增益 */
	}
	wm8978_WriteReg(49, usReg);

	/*	REG 50    (50是左声道，51是右声道，配置寄存器功能一致) WM8978(V4.5_2011).pdf 56页
		B8:6	AUXLMIXVOL = 111	AUX用于FM收音机信号输入
		B5		AUXL2LMIX = 1		Left Auxilliary input to left channel
		B4:2	BYPLMIXVOL			音量
		B1		BYPL2LMIX = 0;		Left bypass path (from the left channel input boost output) to left output mixer
		B0		DACL2LMIX = 1;		Left DAC output to left output mixer
	*/
	usReg = 0;
	if (_InPath & AUX_ON)
	{
		usReg |= ((7 << 6) | (1 << 5));
	}
	if ((_InPath & LINE_ON) || (_InPath & MIC_LEFT_ON) || (_InPath & MIC_RIGHT_ON))
	{
		usReg |= ((7 << 2) | (1 << 1));
	}
	if (_InPath & DAC_ON)
	{
		usReg |= (1 << 0);
	}
	wm8978_WriteReg(50, usReg);
	wm8978_WriteReg(51, usReg);

	/*	R56 寄存器   OUT3 mixer ctrl
		B8:7	0
		B6		OUT3MUTE,  	0 = Output stage outputs OUT3 mixer;  1 = Output stage muted – drives out VMID.
		B5:4	0
		B3		BYPL2OUT3,	OUT4 mixer output to OUT3  (反相)
		B4		0
		B2		LMIX2OUT3,	Left ADC input to OUT3
		B1		LDAC2OUT3,	Left DAC mixer to OUT3
		B0		LDAC2OUT3,	Left DAC output to OUT3
	*/
	usReg = 0;
	if (_OutPath & OUT3_4_ON)
	{
		usReg |= (1 << 3);
	}
	wm8978_WriteReg(56, usReg);

	/* R57 寄存器		OUT4 (MONO) mixer ctrl
		B8:7	0
		B6		OUT4MUTE,	0 = Output stage outputs OUT4 mixer  1 = Output stage muted – drives outVMID.
		B5		HALFSIG,	0 = OUT4 normal output	1 = OUT4 attenuated by 6dB
		B4		LMIX2OUT4,	Left DAC mixer to OUT4
		B3		LDAC2UT4,	Left DAC to OUT4
		B2		BYPR2OUT4,	Right ADC input to OUT4
		B1		RMIX2OUT4,	Right DAC mixer to OUT4
		B0		RDAC2OUT4,	Right DAC output to OUT4
	*/
	usReg = 0;
	if (_OutPath & OUT3_4_ON)
	{
		usReg |= ((1 << 4) |  (1 << 1));
	}
	wm8978_WriteReg(57, usReg);


	/* R11, 12 寄存器 DAC数字音量
		R11		Left DAC Digital Volume
		R12		Right DAC Digital Volume
	*/
	if (_InPath & DAC_ON)
	{
		wm8978_WriteReg(11, 255);
		wm8978_WriteReg(12, 255 | 0x100);
	}
	else
	{
		wm8978_WriteReg(11, 0);
		wm8978_WriteReg(12, 0 | 0x100);
	}

	/*	R10 寄存器 DAC Control
		B8	0
		B7	0
		B6	SOFTMUTE,	Softmute enable:
		B5	0
		B4	0
		B3	DACOSR128,	DAC oversampling rate: 0=64x (lowest power) 1=128x (best performance)
		B2	AMUTE,		Automute enable
		B1	DACPOLR,	Right DAC output polarity
		B0	DACPOLL,	Left DAC output polarity:
	*/
	if (_InPath & DAC_ON)
	{
		wm8978_WriteReg(10, 0);
	}
}

/**
	* @brief  修改输出通道1音量
	* @param  _ucVolume ：音量值, 0-63
	* @retval 无
	*/
void wm8978_SetOUT1Volume(uint8_t _ucVolume)
{
	uint16_t regL;
	uint16_t regR;

	if (_ucVolume > VOLUME_MAX)
	{
		_ucVolume = VOLUME_MAX;
	}

	regL = _ucVolume;
	regR = _ucVolume;

	/*
		R52	LOUT1 Volume control
		R53	ROUT1 Volume control
	*/
	/* 先更新左声道缓存值 */
	wm8978_WriteReg(52, regL | 0x00);

	/* 再同步更新左右声道的音量 */
	wm8978_WriteReg(53, regR | 0x100);	/* 0x180表示 在音量为0时再更新，避免调节音量出现的“嘎哒”声 */
}

/**
	* @brief  配置WM8978的音频接口(I2S)
	* @param  _usStandard : 接口标准，I2S_Standard_Phillips, I2S_Standard_MSB 或 I2S_Standard_LSB
	* @param  _ucWordLen : 字长，16、24、32  （丢弃不常用的20bit格式）
	* @retval 无
	*/
void wm8978_CfgAudioIF(uint16_t _usStandard, uint8_t _ucWordLen)
{
	uint16_t usReg;

	/* WM8978(V4.5_2011).pdf 73页，寄存器列表 */

	/*	REG R4, 音频接口控制寄存器
		B8		BCP	 = X, BCLK极性，0表示正常，1表示反相
		B7		LRCP = x, LRC时钟极性，0表示正常，1表示反相
		B6:5	WL = x， 字长，00=16bit，01=20bit，10=24bit，11=32bit （右对齐模式只能操作在最大24bit)
		B4:3	FMT = x，音频数据格式，00=右对齐，01=左对齐，10=I2S格式，11=PCM
		B2		DACLRSWAP = x, 控制DAC数据出现在LRC时钟的左边还是右边
		B1 		ADCLRSWAP = x，控制ADC数据出现在LRC时钟的左边还是右边
		B0		MONO	= 0，0表示立体声，1表示单声道，仅左声道有效
	*/
	usReg = 0;
	if (_usStandard == I2S_Standard_Phillips)	/* I2S飞利浦标准 */
	{
		usReg |= (2 << 3);
	}
	else if (_usStandard == I2S_Standard_MSB)	/* MSB对齐标准(左对齐) */
	{
		usReg |= (1 << 3);
	}
	else if (_usStandard == I2S_Standard_LSB)	/* LSB对齐标准(右对齐) */
	{
		usReg |= (0 << 3);
	}
	else	/* PCM标准(16位通道帧上带长或短帧同步或者16位数据帧扩展为32位通道帧) */
	{
		usReg |= (3 << 3);;
	}

	if (_ucWordLen == 24)
	{
		usReg |= (2 << 5);
	}
	else if (_ucWordLen == 32)
	{
		usReg |= (3 << 5);
	}
	else
	{
		usReg |= (0 << 5);		/* 16bit */
	}
	wm8978_WriteReg(4, usReg);


	/*
		R6，时钟产生控制寄存器
		MS = 0,  WM8978被动时钟，由MCU提供MCLK时钟
	*/
	wm8978_WriteReg(6, 0x000);
}



