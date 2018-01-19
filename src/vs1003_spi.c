/*
    2017 12 31
	@sococos
*/


#include "vs1003_spi.h"

static spi_t spi_master;
static gpio_t gpio_xdcs;
static gpio_t gpio_xrst;
static gpio_t gpio_dreq;
static gpio_t gpio_xcs;

void vs1003_hdl_init()
{
  	spi_init(&spi_master, SPI0_MOSI, SPI0_MISO, SPI0_SCLK, SPI0_CS);
}

void vs1003_hdl_spi_init(uint32_t speed, uint8_t bitwidth)
{
	spi_format(&spi_master, bitwidth, 3, 0);  //master
	spi_frequency(&spi_master, speed);
}

void vs1003_hdl_gpio_init()
{
  	gpio_init(&gpio_xdcs, SPI0_XDCS);
	gpio_dir(&gpio_xdcs, PIN_OUTPUT);
	gpio_mode(&gpio_xdcs, PullNone);
	
	gpio_init(&gpio_xrst, SPI0_XRST);
	gpio_dir(&gpio_xrst, PIN_OUTPUT);
	gpio_mode(&gpio_xrst, PullNone);
	
	gpio_init(&gpio_dreq, SPI0_DREQ);
	gpio_dir(&gpio_dreq, PIN_INPUT);
	gpio_mode(&gpio_dreq, PullUp);
	
	gpio_init(&gpio_xcs, SPI0_XCS);
	gpio_dir(&gpio_xcs, PIN_OUTPUT);
	gpio_mode(&gpio_xcs, PullNone);
}

void vs1003_hdl_xrst(uint8_t v)
{
	gpio_write(&gpio_xrst, v);
	
}

void vs1003_hdl_xcs(uint8_t v)
{
	gpio_write(&gpio_xcs, v);
}

void vs1003_hdl_xdcs(uint8_t v)
{
	gpio_write(&gpio_xdcs, v);
}

uint8_t vs1003_hdl_dreq()
{
  	return gpio_read(&gpio_dreq);
}

uint32_t vs1003_hdl_spi_rw(uint32_t ch)
{
	return spi_master_write(&spi_master, ch);
}

