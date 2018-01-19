#ifndef __K24CXX_H
#define __K24CXX_H

#include "gpio_api.h"
#include "i2c_api.h"
#include "app_debug.h"

//IO方向设置
#define MBED_I2C_MTR_SCL    PB_2
#define MBED_I2C_MTR_SDA    PB_3


//IO操作函数	 
#define IIC_SCL_H    gpio_write(&i2c_gpio_scl, 1) //SCL
#define IIC_SCL_L    gpio_write(&i2c_gpio_scl, 0)

#define IIC_SDA_H    gpio_write(&i2c_gpio_sda, 1) //SDA
#define IIC_SDA_L    gpio_write(&i2c_gpio_sda, 0) //SDA

#define IIC_SDA_READ     gpio_read(&i2c_gpio_sda)//输入SDA 

#define AT24C01		127
#define AT24C02		255
#define AT24C04		511
#define AT24C08		1023
#define AT24C16		2047
#define AT24C32		4095
#define AT24C64	        8191
#define AT24C128	16383
#define AT24C256	32767  

#define EEROM_PAGESIZE  32

#define EEROM_SYS_NAME   3936      //存储名
#define EEPROM_FIRSTADR 3954       //APP更新后标志地址   

#define APPFIRSTFLAG 0x5A       //APP更新后标志 

//Mini STM32开发板使用的是24c02，所以定义EE_TYPE为AT24C02
#define EE_TYPE AT24C32


//IIC所有操作函数
void IIC_Init(void);                           //IIC的IO口				 
void IIC_Start(void);				//IIC开始信号
void IIC_Stop(void);	  			//IIC停止信号
uint8_t IIC_Send_Byte(u8 sdata);			//IIC发送一个字节
uint8_t IIC_Wait_Ack(void); 		        //IIC等待ACK信号
void IIC_Ack(void);			        //IIC发送ACK信号
void IIC_NAck(void);				//IIC不发送ACK信号
u8 IIC_Read_Byte(uint8_t nend);


uint8_t AT24CXX_ReadOneByte(u16 raddr,u8* rdata);
uint8_t AT24CXX_ReadLenByte(u16 raddr,u8* rdata,u16 len);

uint8_t AT24CXX_WriteOneByte(u16 waddr,u8  wdata);
uint8_t AT24CXX_WriteLenByte(u16 waddr,u8 *wdata,u16 len);
#endif
