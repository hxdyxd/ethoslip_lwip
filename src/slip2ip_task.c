#include "FreeRTOS.h"
#include "task.h"
#include "lwip/raw.h"
#include "pbuf.h"

#include "slip.h"
#include "app_debug.h"
#include "tun.h"

#include "wlan_api.h"

#define CHECK_SUM_ON   1

#define TUN_SERVER "67.209.189.217"
#define SERVER_PORT 2020
#define BUF_SIZE 2000

static TaskHandle_t slip2ip_task_handle = NULL;
static struct udp_pcb *udp_sock = NULL;
static unsigned char key[BUF_SIZE];
static unsigned char udp_out_buffer[BUF_SIZE];
static unsigned char slip_out_buffer[BUF_SIZE];

#if CHECK_SUM_ON
//Checksum
int if_api_calculate_checksum(void *buf, unsigned int len);
int if_api_check(void *buf, unsigned int len);
#endif

void udp_data_input_cb(void *arg,
				  struct udp_pcb *upcb,
				  struct pbuf *p,
				  struct ip_addr *addr,
				  u16_t port)
{
  	//printf("udp len = %d \n", p->tot_len);
  	uint16_t total_len = p->tot_len;
  	pbuf_copy_partial(p, udp_out_buffer, total_len, 0);
	key_xor(udp_out_buffer, total_len, key);
	
#if CHECK_SUM_ON
    total_len = if_api_calculate_checksum(udp_out_buffer, total_len);
#endif
	
	send_packet(udp_out_buffer, total_len);
  	pbuf_free(p);
}


void slip2ip_task_proc(void *par)
{
	struct ip_addr s_ip;
	
	
/*	uint8_t buf[256];
	for(int i=0;i<256;i++) {
		buf[i] = i;
	}
	send_packet(buf, 256);

	while(1) {
		send_packet(buf, 256);
	}

	return;*/
	
	key_init(key);
	
	udp_sock = udp_new();
	if(udp_sock == NULL) {
	  	APP_ERROR("Failed to new udp pcb\n");
	}
	
	s_ip.addr = inet_addr(TUN_SERVER);
	if(ERR_OK != udp_connect(udp_sock, &s_ip, SERVER_PORT) ) {
	  	APP_ERROR("Failed to connect udp server\n");
	}
	
	udp_recv(udp_sock, udp_data_input_cb, NULL);
	
	while(1) {
		int total_len = recv_packet(slip_out_buffer, BUF_SIZE);
		//total_len -= 2;
		//printf("slip len = %d \n", total_len);
		
#if CHECK_SUM_ON
		total_len = if_api_check(slip_out_buffer, total_len);
		if(total_len < 0) {
			APP_DEBUG("checksum error\n");
			continue;
		}
#endif
		
		if(total_len > 14 && slip_out_buffer[12] == 0xff
		   && slip_out_buffer[13] == 0xff) {
		  	wlan_api_pack_proc(slip_out_buffer, total_len);
			continue;
		}
		
		key_xor(slip_out_buffer, total_len, key);
		
		struct pbuf *p = NULL;
		 p = pbuf_alloc(PBUF_TRANSPORT, total_len, PBUF_POOL);
		if (p == NULL) {
			printf("Cannot allocate pbuf to receive packet\n");
			continue;
		}
		
		err_t err = pbuf_take(p, slip_out_buffer, total_len);
		if(err != ERR_OK) {
		  	printf("Cannot take pbuf\n");
			pbuf_free(p);
			continue;
		}
		
		udp_send(udp_sock, p);
		pbuf_free(p);
	}
}

void slip2ip_task_start(void)
{
  	APP_DEBUG("Run slip2ip_task %08X\n", (uint32_t)slip2ip_task_proc );
	xTaskCreate(slip2ip_task_proc,
				"slip_task",
				1024,
				NULL,
				(configMAX_PRIORITIES-3),
				&slip2ip_task_handle
	);
}

#if CHECK_SUM_ON
int if_api_calculate_checksum(void *buf, unsigned int len)
{
	unsigned char *p = (unsigned char *)buf;
	unsigned int sum = 0;
	unsigned int i;
	for(i=0;i<len;i++,p++)
		sum += *p;
#ifdef DEBUG_CHECKSUM
	printf("sum = %d \r\n", sum);
#endif
	sum = (sum % 65536) ^ 0xffff;
#ifdef DEBUG_CHECKSUM
	printf("rev.hex = 0x%04x \r\n", sum);
#endif
	*p = (sum & 0x7f00) >> 8;
	p++;
	*p = sum & 0x7f;
	return len + 2;
}

int if_api_check(void *buf, unsigned int len)
{
	unsigned char *p = (unsigned char *)buf;
	if(len < 2)
		return -1;
	uint8_t chechsum_low = p[len-1];
	uint8_t chechsum_high = p[len-2];
	p[len-1] = 0;
	p[len-2] = 0;
	if_api_calculate_checksum(p, len-2);

	if(chechsum_low != p[len-1] || chechsum_high != p[len-2])
		return -1;
	return len - 2;
}
#endif




