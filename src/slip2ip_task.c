#include "FreeRTOS.h"
#include "task.h"
#include "lwip/raw.h"
#include "lwip/sys.h"
#include "pbuf.h"
#include "slip.h"
#include "app_debug.h"
#include "tun.h"

#include "wlan_api.h"

#ifndef _USE_UDP_TUNNEL
	#define _USE_UDP_TUNNEL 0
#endif


#define CHECK_SUM_ON    1
#define BUF_SIZE        2000

static TaskHandle_t slip2ip_task_handle = NULL;
static unsigned char slip_out_buffer[BUF_SIZE];
static unsigned int slip_used = 0;

#if CHECK_SUM_ON
//Checksum
int if_api_calculate_checksum(void *buf, unsigned int len);
int if_api_check(void *buf, unsigned int len);
#endif

#if _USE_UDP_TUNNEL	

#define TUN_SERVER "67.209.189.217"
#define SERVER_PORT 2020
static struct udp_pcb *udp_sock = NULL;
static unsigned char key[BUF_SIZE];
static unsigned char udp_out_buffer[BUF_SIZE];


void udp_data_input_cb(void *arg,
				  struct udp_pcb *upcb,
				  struct pbuf *p,
				  struct ip_addr *addr,
				  u16_t port)
{
//  printf("udp len = %d \n", p->tot_len);
  	if(slip_used) {
	  	pbuf_free(p);
		return;
	}
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
	
	key_init(key);
	
	udp_sock = udp_new();
	if(udp_sock == NULL) {
	  	APP_ERROR("Failed to new udp pcb\n");
		return;
	}
	
	s_ip.addr = inet_addr(TUN_SERVER);
	if(ERR_OK != udp_connect(udp_sock, &s_ip, SERVER_PORT) ) {
	  	APP_ERROR("Failed to connect udp server\n");
		return;
	}
	
	udp_recv(udp_sock, udp_data_input_cb, NULL);
	
	while(1) {
		int total_len = recv_packet(slip_out_buffer, BUF_SIZE);
		//printf("slip len = %d \n", total_len);
		
#if CHECK_SUM_ON
		total_len = if_api_check(slip_out_buffer, total_len);
		if(total_len < 0) {
			APP_WARN("checksum error\n");
			continue;
		}
#endif
		
		if(IS_WLAN_API_FRAME_LEN(total_len) 
		&& IS_WLAN_API_FRAME(slip_out_buffer)) {
		  	int len = wlan_api_pack_proc(slip_out_buffer, total_len);
			//reply data 
			if(len > 0) {
#if CHECK_SUM_ON
    			len = if_api_calculate_checksum(slip_out_buffer, len);
#endif			  	
				slip_used = 1;
				send_packet(slip_out_buffer, len);
				slip_used = 0;
			}
			continue;
		}
		
		key_xor(slip_out_buffer, total_len, key);
		
		struct pbuf *p = NULL;
		 p = pbuf_alloc(PBUF_TRANSPORT, total_len, PBUF_POOL);
		if (p == NULL) {
			APP_ERROR("Cannot allocate pbuf to receive packet\n");
			continue;
		}
		
		err_t err = pbuf_take(p, slip_out_buffer, total_len);
		if(err != ERR_OK) {
		  	APP_ERROR("Cannot take pbuf\n");
			pbuf_free(p);
			continue;
		}
		
		udp_send(udp_sock, p);
		pbuf_free(p);
	}
}

//direct connect
#else

extern int skbbuf_used_num, max_local_skb_num;
extern int max_skb_buf_num, skbdata_used_num;
#include "lwip_intf.h"
#include "ethernetif.h"
static unsigned char from_ap_out_buffer[BUF_SIZE];


err_t from_ap_input(struct pbuf *p, struct netif *inp)
{
	if(slip_used) {
	  	pbuf_free(p);
		return ERR_OK;
	}
    if ( max_local_skb_num - 2 <= skbbuf_used_num || max_skb_buf_num - 2 <= skbdata_used_num ) {
	  	pbuf_free(p);
	  	//PRINTF("lost pack\n");
	  	return ERR_OK;
	}
	uint16_t total_len = p->tot_len;
  	pbuf_copy_partial(p, from_ap_out_buffer, total_len, 0);
	
#if CHECK_SUM_ON
    total_len = if_api_calculate_checksum(from_ap_out_buffer, total_len);
#endif
	
	send_packet(from_ap_out_buffer, total_len);
	pbuf_free(p);
	return ERR_OK;
}

void slip2ip_task_proc(void *par)
{
	while(1) {
		int total_len = recv_packet(slip_out_buffer, BUF_SIZE);
//		printf("slip len = %d \n", total_len);
		
#if CHECK_SUM_ON
		total_len = if_api_check(slip_out_buffer, total_len);
		if(total_len < 0) {
			APP_WARN("checksum error\n");
			continue;
		}
#endif
		
		if(IS_WLAN_API_FRAME_LEN(total_len) 
		&& IS_WLAN_API_FRAME(slip_out_buffer)) {
		  	int len = wlan_api_pack_proc(slip_out_buffer, total_len);
			//reply data 
			if(len > 0) {
#if CHECK_SUM_ON
    			len = if_api_calculate_checksum(slip_out_buffer, len);
#endif			  	
				slip_used = 1;
				send_packet(slip_out_buffer, len);
				slip_used = 0;
			}
			continue;
		}
		
		struct pbuf *p = NULL;
		 p = pbuf_alloc(PBUF_RAW, total_len, PBUF_POOL);
		if (p == NULL) {
			APP_ERROR("Cannot allocate pbuf to receive packet\n");
			continue;
		}
		
		err_t err = pbuf_take(p, slip_out_buffer, total_len);
		if(err != ERR_OK) {
		  	APP_ERROR("Cannot take pbuf\n");
			pbuf_free(p);
			continue;
		}
		
		low_level_output(&xnetif[0], p);
		pbuf_free(p);
	}
}
#endif

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




