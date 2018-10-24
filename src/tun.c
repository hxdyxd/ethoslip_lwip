#include "tun.h"

static TaskHandle_t tun_task_handle = NULL;
static uint32_t total_pack_up = 0;
static uint32_t lost_pack_up = 0;
static uint32_t total_pack_down = 0;
static uint32_t lost_pack_down = 0;


int key_init(char *key)
{
    int i;
    for(i=0;i<BUF_SIZE;i++) {
        key[i] = (unsigned char)(i*i + 128);
        //printf("%d ", key[i]);
    }
    //printf("\n");
    return 0;
}

int key_xor(char *buff, int len, char *key)
{
    int i;
    int j = BUF_SIZE - len;
    for(i=0; i<len; i++) {
        buff[i] = buff[i] ^ key[j++];
    }
    return 0;
}

static u16_t sbuf_key_xor(struct pbuf *buf, const char *key)
{
  	struct pbuf *p;
  	u16_t total_len = 0;
	u16_t len = buf->tot_len;
    u16_t j = BUF_SIZE - len;
	
	u16_t buf_copy_len;
	
	for(p = buf; len != 0 && p != NULL; p = p->next) {
		/* copy from this buffer. maybe only partially. */
		buf_copy_len = p->len;
		if (buf_copy_len > len)
			buf_copy_len = len;
		/* copy the necessary parts of the buffer */
		unsigned char *buff = p->payload;
		for(u16_t k = 0; k < buf_copy_len; k++, total_len++) {
			buff[k] = buff[k] ^ key[j++];
		}
		len -= buf_copy_len;
    }
    return total_len;
}

static struct udp_pcb *udp_sock = NULL;
static unsigned char key[BUF_SIZE];
extern int skbbuf_used_num, max_local_skb_num;
extern int max_skb_buf_num, skbdata_used_num;
#include "lwip_intf.h"

void tun_task_proc(void *par)
{
  	struct ip_addr s_ip;
	key_init(key);
	
	udp_sock = udp_new();
	if(udp_sock == NULL) {
	  	APP_ERROR("Failed to new udp pcb\n");
	}
	
//	APP_DEBUG("ok\n");
	
	//s_ip.addr = 0;
	if(ERR_OK != udp_bind(udp_sock, IP_ADDR_ANY, LOCAL_PORT) ) {
	  	APP_ERROR("Failed to bind udp port\n");
	}
	
//	APP_DEBUG("ok\n");
	
	s_ip.addr = inet_addr(TUN_SERVER);
	if(ERR_OK != udp_connect(udp_sock, &s_ip, SERVER_PORT) ) {
	  	APP_ERROR("Failed to connect udp server\n");
	}
//	APP_DEBUG("ok\n");
	
	udp_recv(udp_sock, udp_input_cb, NULL);
	
//	APP_DEBUG("ok\n");
//  vTaskDelete(NULL);
	
	while(2) {
	  	vTaskDelay(10000);
		APP_DEBUG("max_local_skb_num = %d, max_skb_buf_num = %d\n", max_local_skb_num, max_skb_buf_num);
		APP_DEBUG(" up pack (lost)num   = (%d) %d\n", lost_pack_up, total_pack_up);
		APP_DEBUG(" down pack (lost)num = (%d) %d\n", lost_pack_down, total_pack_down);
	}
}

void tun_task_start(void)
{
	total_pack_up = 0;
	lost_pack_up = 0;
	total_pack_down = 0;
	lost_pack_down = 0;
  	APP_DEBUG("Run task %08X\n", (uint32_t)tun_task_proc );
	xTaskCreate(tun_task_proc,
				"tun_task",
				1024,
				NULL,
				1,
				&tun_task_handle
	);
}

void udp_input_cb(void *arg, struct udp_pcb *upcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
	total_pack_down++;
  	if ( max_local_skb_num - 2 <= skbbuf_used_num || max_skb_buf_num - 2 <= skbdata_used_num ) {
	  	pbuf_free(p);
		lost_pack_down++;
	  	//PRINTF("lost pack\n");
	  	return;
		//vTaskDelay(1);
	}
	sbuf_key_xor(p, key);
	
	if(p->len >= 12) {
	  	memcpy(((uint8_t *)p->payload)+6, xnetif[1].hwaddr, xnetif[1].hwaddr_len);
	}
	low_level_output(&xnetif[1], p);  //output ap netif
	pbuf_free(p);
}

#define MAX_SKB_BUF_NUM      20  //fix
#define MAX_LOCAL_SKB_NUM    (MAX_SKB_BUF_NUM + 2)

err_t tun_input(struct pbuf *p, struct netif *inp)
{
  	total_pack_up++;
    if ( max_local_skb_num - 2 <= skbbuf_used_num || max_skb_buf_num - 2 <= skbdata_used_num ) {
	  	pbuf_free(p);
		lost_pack_up++;
	  	//PRINTF("lost pack\n");
	  	return ERR_OK;
		//vTaskDelay(1);
	}
  	sbuf_key_xor(p, key);
	udp_sendto_if(udp_sock, p, &udp_sock->remote_ip, udp_sock->remote_port, xnetif);  //output sta netif
	pbuf_free(p);
	return ERR_OK;
}

