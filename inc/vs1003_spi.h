#ifndef _VS1003_SPI_H_
#define _VS1003_SPI_H_

#include "FreeRTOS.h"
#include "spi_api.h"
#include "gpio_api.h"
#include "gpio_irq_api.h"
#include "stdint.h"


#define SPI0_MOSI  PC_2
#define SPI0_MISO  PC_3
#define SPI0_SCLK  PC_1
#define SPI0_CS    PC_0

#define SPI0_XDCS  PB_2
#define SPI0_XRST  PB_3
#define SPI0_DREQ  PC_5
#define SPI0_XCS   PC_4

#define SLEEP_MS(v) vTaskDelay(v)
#define VS_XRST(v) vs1003_hdl_xrst(v)
#define VS_XCS(v) vs1003_hdl_xcs(v)
#define VS_XDCS(v) vs1003_hdl_xdcs(v)
#define VS_DQ vs1003_hdl_dreq()
#define VS_SPI_ReadWriteByte(v) vs1003_hdl_spi_rw(v)

void vs1003_hdl_init();
void vs1003_hdl_gpio_init();
void vs1003_hdl_spi_init(uint32_t speed, uint8_t bitwidth);
void vs1003_hdl_xrst(uint8_t v);
void vs1003_hdl_xcs(uint8_t v);
void vs1003_hdl_xdcs(uint8_t v);
uint8_t vs1003_hdl_dreq();
uint32_t vs1003_hdl_spi_rw(uint32_t ch);

#endif
