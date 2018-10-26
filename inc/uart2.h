#ifndef __UART2_H__
#define __UART2_H__



void uart2_write_string(char *pstr);
void uart2_init(unsigned int rate);
void send_char(unsigned char ch);
void send_char_do(void);
unsigned char recv_char(void);


#endif
