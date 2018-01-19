#include "K24cxx.h"


gpio_t i2c_gpio_scl;
gpio_t i2c_gpio_sda;

/*******************************************************************************
* 名称: 
* 功能: 
* 形参:       
* 返回: 无
* 说明: 
*******************************************************************************/
//#pragma optimize=none 
//void delay_us(uint16_t nCount)
//{
//  u32 del = nCount*8;
//  //48M 0.32uS
//  //24M 0.68uS
//  //16M 1.02us
//  while(del--);
//}

/*******************************************************************************
* 名称: IIC_Init()
* 功能: 
* 形参:       
* 返回: 无
* 说明: 
*******************************************************************************/
void IIC_Init(void)
{
  	gpio_set(MBED_I2C_MTR_SCL);
	gpio_set(MBED_I2C_MTR_SDA);
	gpio_init(&i2c_gpio_scl, MBED_I2C_MTR_SCL);
	gpio_dir(&i2c_gpio_scl, PIN_OUTPUT);
	gpio_mode(&i2c_gpio_scl, OpenDrain);
	
	gpio_init(&i2c_gpio_sda, MBED_I2C_MTR_SDA);
	gpio_dir(&i2c_gpio_sda, PIN_INPUT);
	gpio_mode(&i2c_gpio_sda, OpenDrain);
	
#if (SYSINFOR_PRINTF == 1)
  printf("IIC_Init\r\n");
#endif
}
/*******************************************************************************
* 名称: IIC_delay()
* 功能: 
* 形参:       
* 返回: 
* 说明: 
*******************************************************************************/
void IIC_delay(void) 
{ 
  u8 i=25; //这里可以优化速度 ，经测试最低到5还能写入 
  while(i)  
  {  
    i--;  
  }  
} 

/*******************************************************************************
* 名称: IIC_Start()
* 功能: 产生IIC起始信号
* 形参:       
* 返回: 
* 说明: 
*******************************************************************************/
void IIC_Start(void)
{
  IIC_SDA_H; 
  IIC_SCL_H; 
  IIC_delay(); 
  if(!IIC_SDA_READ) {
		//APP_ERROR("\n");
		return; //SDA线为低电平则总线忙,退出 
  }
  IIC_SDA_L; 
  IIC_delay(); 
  if(IIC_SDA_READ) {
		//APP_ERROR("\n");
		return; //SDA线为高电平则总线出错,退出 
  }
  IIC_SCL_L; 
  IIC_delay(); 
}	  
/*******************************************************************************
* 名称: IIC_Stop()
* 功能: 产生IIC停止信号
* 形参:       
* 返回: 
* 说明: 
*******************************************************************************/
void IIC_Stop(void)
{
  IIC_SCL_L; 
  IIC_delay(); 
  IIC_SDA_L; 
  IIC_delay(); 
  IIC_SCL_H; 
  IIC_delay(); 
  IIC_SDA_H; 
  IIC_delay(); 
  IIC_SCL_L;
}      
/*******************************************************************************
* 名称: IIC_Wait_Ack()
* 功能: 应答信号
* 形参:       
* 返回: 
* 说明: true接收应答成功 false接收应答失败
*******************************************************************************/
uint8_t IIC_Wait_Ack(void)
{
  IIC_SCL_L; 
  IIC_delay(); 
  IIC_SDA_H; 
  IIC_delay(); 
  IIC_SCL_H; 
  IIC_delay(); 
  if(IIC_SDA_READ)//无应答
  { 
    IIC_SCL_L; 
	//APP_ERROR("\n");
    return FALSE; 
  } 
  IIC_SCL_L; 
  return TRUE; 
} 
/*******************************************************************************
* 名称: IIC_Ack()
* 功能: ACK应答
* 形参:       
* 返回: 
* 说明: 
*******************************************************************************/
void IIC_Ack(void)
{
  IIC_SCL_L; 
  IIC_delay(); 
  IIC_SDA_L; 
  IIC_delay(); 
  IIC_SCL_H; 
  IIC_delay(); 
  IIC_SCL_L; 
  IIC_delay(); 
}
/*******************************************************************************
* 名称: IIC_NAck()
* 功能: 不产生ACK应答
* 形参:       
* 返回: 
* 说明: 
*******************************************************************************/		    
void IIC_NAck(void)
{
  IIC_SCL_L; 
  IIC_delay(); 
  IIC_SDA_H; 
  IIC_delay(); 
  IIC_SCL_H; 
  IIC_delay(); 
  IIC_SCL_L; 
  IIC_delay(); 
}
/*******************************************************************************
* 名称: IIC_Send_Byte()
* 功能: IIC发送一个字节
* 形参:       
* 返回: 
* 说明: true ok，false nok
*******************************************************************************/			  
uint8_t IIC_Send_Byte(u8 sdata)
{                        
  u8 i=8; 
  
  while(i--) 
  { 
    IIC_SCL_L; 
    IIC_delay(); 
    if(sdata&0x80)
	  	IIC_SDA_H; 
    else
	  	IIC_SDA_L;   
    sdata<<=1; 
    IIC_delay(); 
    IIC_SCL_H; 
    IIC_delay(); 
  } 
  IIC_SCL_L; 
  
  return (IIC_Wait_Ack());
} 	    
/*******************************************************************************
* 名称: IIC_NAck()
* 功能: 读1个字节
* 形参: 发送ACK      
* 返回: 
* 说明: nend = TRUE 继续
*******************************************************************************/
u8 IIC_Read_Byte(uint8_t nend)
{
  u8 i=8; 
  u8 ReceiveByte=0; 
  
  IIC_SDA_H; 
  while(i--) 
  { 
    ReceiveByte<<=1;       
    IIC_SCL_L; 
    IIC_delay(); 
    IIC_SCL_H; 
    IIC_delay(); 
    if(IIC_SDA_READ) 
    { 
      ReceiveByte|=0x01; 
    } 
  } 
  IIC_SCL_L;					 
  if(nend)
    IIC_Ack();  //发送ACK
  else
    IIC_NAck(); //发送NACK   
  return ReceiveByte;
}


/*******************************************************************************
* 名称: AT24CXX_ReadOneByte()
* 功能: 在AT24CXX指定地址读出数据
* 形参: ReadAddr:开始读数的地址      
* 返回: 
* 说明: 读到的数据 
*******************************************************************************/
uint8_t AT24CXX_ReadOneByte(u16 raddr,u8* rdata)
{				  		  	    																 
  IIC_Start();  
  if(EE_TYPE>AT24C16)
  {
    if(IIC_Send_Byte(0xA0) == FALSE){//发送写命令
      IIC_Stop();
      return FALSE;
    }
    if(IIC_Send_Byte(raddr>>8) == FALSE){//发送写命令
      IIC_Stop();
      return FALSE;
    }		 
  }else{
    if(IIC_Send_Byte(0XA0+((raddr>>8)<<1)) == FALSE){//发送写命令
      IIC_Stop();
      return FALSE;
    }
  }
  if(IIC_Send_Byte(raddr) == FALSE){   //发送低地址
    IIC_Stop();
    return FALSE;
  }	    
  IIC_Start();  
  if(IIC_Send_Byte(0xA1) == FALSE){   //进入接收模式
    IIC_Stop();
    return FALSE;
  } 
  *rdata = IIC_Read_Byte(FALSE);		   
  IIC_Stop();	    
  return TRUE;
}
/*******************************************************************************
* 名称: AT24CXX_WriteOneByte()
* 功能: 在AT24CXX指定地址写入一个数据
* 形参: DataToWrite:要写入的数据   WriteAddr  :写入数据的目的地址    
* 返回: 
* 说明: 
*******************************************************************************/
uint8_t AT24CXX_WriteOneByte(u16 waddr,u8 wdata)
{				   	  	    																 
  IIC_Start();  
  if(EE_TYPE>AT24C16)
  {
    if(IIC_Send_Byte(0xA0) == FALSE){    //选择I2C
      IIC_Stop();
      return FALSE;
    }
    if(IIC_Send_Byte(waddr>>8) == FALSE){//高八位地址
      IIC_Stop();
      return FALSE;
    }
  }else
  {
    if(IIC_Send_Byte(0XA0+((waddr>>8)<<1)) == FALSE){//发送写命令
      IIC_Stop();
      return FALSE;
    }
  }	 	   
  if(IIC_Send_Byte(waddr) == FALSE){   //发送低地址
    IIC_Stop();
    return FALSE;
  } 										  		   
  if(IIC_Send_Byte(wdata) == FALSE){   //发送DATA
    IIC_Stop();
    return FALSE;
  } 		    	   
  IIC_Stop();//产生一个停止条件 
  return TRUE;
}
/*******************************************************************************
* 名称: AT24CXX_WriteLenByte()
* 功能: 写len数据
* 形参: waddr wdata len
* 返回: 
* 说明: 
*******************************************************************************/
uint8_t AT24CXX_WriteLenByte(u16 waddr,u8 *wdata,u16 len)
{  	
  IIC_Start();  
  if(EE_TYPE>AT24C16)
  {
    if(IIC_Send_Byte(0xA0) == FALSE){    //选择I2C
      IIC_Stop();
      return FALSE;
    }
    if(IIC_Send_Byte(waddr>>8) == FALSE){//高八位地址
      IIC_Stop();
      return FALSE;
    }
  }else
  {
    if(IIC_Send_Byte(0XA0+((waddr>>8)<<1)) == FALSE){//发送写命令
      IIC_Stop();
      return FALSE;
    }
  }	 	   
  if(IIC_Send_Byte(waddr) == FALSE){   //发送低地址
    IIC_Stop();
    return FALSE;
  }
  for(;len>0;len--){
    if(IIC_Send_Byte(*wdata++) == FALSE){     //发送字节
      IIC_Stop();
      return FALSE;
    }
  }		    	   
  IIC_Stop();//产生一个停止条件 
  return TRUE;												    
}
/*******************************************************************************
* 名称: AT24CXX_ReadLenByte()
* 功能: 读len字节数据
* 形参: raddr  rdata len
* 返回: 
* 说明: 
*******************************************************************************/
uint8_t AT24CXX_ReadLenByte(u16 raddr,u8 *rdata,u16 len)
{  	
  IIC_Start();  
  if(EE_TYPE>AT24C16)
  {
    if(IIC_Send_Byte(0xA0) == FALSE){//发送写命令
      IIC_Stop();
      return FALSE;
    }
    if(IIC_Send_Byte(raddr>>8) == FALSE){//发送写命令
      IIC_Stop();
      return FALSE;
    }		 
  }else{
    if(IIC_Send_Byte(0XA0+((raddr>>8)<<1)) == FALSE){//发送写命令
      IIC_Stop();
      return FALSE;
    }
  }
  if(IIC_Send_Byte(raddr) == FALSE){   //发送低地址
    IIC_Stop();
    return FALSE;
  }	    
  IIC_Start();  
  if(IIC_Send_Byte(0xA1) == FALSE){   //进入接收模式
    IIC_Stop();
    return FALSE;
  } 
  while(len != 0){
    len--;
    if(len == 0)*rdata = IIC_Read_Byte(FALSE); //FALSE 读取完成
    else        *rdata++ = IIC_Read_Byte(TRUE);//TRUE  未读完
    
  }
  IIC_Stop();	
  
  return TRUE;
}

