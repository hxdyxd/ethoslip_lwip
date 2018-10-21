#include "device.h"
#include "serial_api.h"
#include "serial_ex_api.h"

#include "FreeRTOS.h"
#include "queue.h"

#define QUEUE_MAX_NUM   (2000)

#define UART2_TX    PA_4
#define UART2_RX    PA_0
serial_t sobj;
xQueueHandle charQueue;


#if 0
static unsigned char gs_tx_buff[QUEUE_MAX_NUM];
static unsigned short gs_tx_ptr = 0;
void send_char(unsigned char ch)
{
  	if(gs_tx_ptr < QUEUE_MAX_NUM) {
  		gs_tx_buff[gs_tx_ptr++] = ch;
  	}
}

void send_char_do(void)
{
  	serial_send_blocked(&sobj, gs_tx_buff, gs_tx_ptr, 100);
	gs_tx_ptr = 0;
}
#else
void send_char(unsigned char ch)
{
  	serial_putc(&sobj, ch);
}

void send_char_do(void)
{
}
#endif


unsigned char recv_char(void)
{
  	unsigned char ch;
	while( xQueueReceive(charQueue, (void *)&ch, 100/portTICK_RATE_MS )
		  != pdPASS)
	{ }
  	return ch;
}


void uart2_write_string(char *pstr)
{
    unsigned int i=0;

    while (*(pstr+i) != 0) {
        serial_putc(&sobj, *(pstr+i));
        i++;
    }
}

void  uart2_irq_handler(uint32_t id, SerialIrq event)
{
  	if(event == RxIrq) {
	  	unsigned char ch = serial_getc(&sobj);
		xQueueSendFromISR( charQueue, (void *)&ch, 0 );
  	}
}

void uart2_init(void)
{
    // mbed uart test
  	/* 建立队列 */
    charQueue = xQueueCreate( QUEUE_MAX_NUM , sizeof(unsigned char) );
	if( charQueue == 0 )
	{
		// Queue was not created and must not be used.
	  	printf("Queue was not created \n");
	}
  
    serial_init(&sobj, UART2_TX, UART2_RX);
    serial_baud(&sobj, 500000);
    serial_format(&sobj, 8, ParityNone, 1);
	
	serial_irq_handler(&sobj, uart2_irq_handler, 0);
	serial_irq_set(&sobj, RxIrq, 1);

    uart2_write_string("UART API Demo...\r\n");
    uart2_write_string("Hello World!!\r\n");
}

