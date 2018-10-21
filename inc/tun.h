#ifndef _TUN_H_
#define _TUN_H_


#include <stdint.h>
#include "app_debug.h"
#include <lwip/sockets.h>
#include "lwip/udp.h"
#include "pbuf.h"
#include "ethernetif.h"

//#define TUN_SERVER "120.79.154.122"
#define TUN_SERVER "67.209.189.217"

#define BUF_SIZE 2000
#define SERVER_PORT 2020
#define LOCAL_PORT (SERVER_PORT-1)

extern struct netif xnetif[];


int key_init(char *key);
int key_xor(char *buff, int len, char *key);

void tun_task_start(void);
err_t tun_input(struct pbuf *p, struct netif *inp);
void udp_input_cb(void *arg, struct udp_pcb *upcb, struct pbuf *p, struct ip_addr *addr, u16_t port);


#endif
