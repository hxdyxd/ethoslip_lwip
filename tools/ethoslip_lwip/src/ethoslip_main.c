/*
 ============================================================================
 Name        : ethoslip_main.c
 Author      : hxdyxd
 Version     :
 Copyright   : Your copyright notice
 Description : ethoslip_main
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "uart2.h"
#include "queue.h"
#include "ethoslipif.h"
#include "app_debug.h"
#include "lwip/timeouts.h"
#include "soft_timer.h"
#include <pthread.h>
#include "wlan_api.h"


static unsigned char slip_out_buffer[BUF_SIZE];
struct netif netif;
//Slip
static volatile uint8_t slip_out_task_run_flag = 0;
static pthread_t gs_slip_out_task_pthread_id;
static pthread_mutex_t gs_slip_out_task_Q_mutex;
static SqQueue Q;

#define lock_interrupts()    pthread_mutex_lock(&gs_slip_out_task_Q_mutex)
#define unlock_interrupts()  pthread_mutex_unlock(&gs_slip_out_task_Q_mutex)


//Slip
int slip_out_task_start(SqQueue *Q);
int slip_out_task_stop(void);
void *slip_out_task_proc(void *par);

extern void user_init(void);
extern void user_loop(void);

int main(int argc, char *argv[]) {

	if(argc < 3) {
		APP_DEBUG("Usage: %s /dev/ttyUSB0 115200\n\n", argv[0]);
		return 0;
	}

	APP_DEBUG("Open UART device %s %d\n", argv[1], atoi(argv[2]) );
	int err = uart2_init(argv[1], atoi(argv[2]));
	if(err < 0) {
		APP_ERROR("UART init failed\n\n");
		return 0;
	}

	lwip_init();
	netif_add(&netif, NULL, NULL, NULL, NULL, ethoslipif_low_level_init, netif_input);
	netif.name[0] = 'e';
	netif.name[1] = '0';
	netif_create_ip6_linklocal_address(&netif, 1);
	netif.ip6_autoconfig_enabled = 1;
	netif_set_default(&netif);  //set default netif
	netif_set_up(&netif);

	netif_set_link_up(&netif);

	soft_timer_init();
	user_init();

	Q_Init(&Q);
	pthread_mutex_init(&gs_slip_out_task_Q_mutex, NULL);
	slip_out_task_start(&Q);

	while(1) {
		lock_interrupts();
		struct pbuf* p = queue_try_get(&Q);
		unlock_interrupts();
		if(p != NULL) {
			//api frame
			if(wlan_api_rx_proc(p->payload, p->len) == 0) {
				pbuf_free(p);
				APP_DEBUG("receive wlan api frame\n");
				continue;
			}
			//data frame
			LINK_STATS_INC(link.recv);
			if(netif.input(p, &netif) != ERR_OK) {
				pbuf_free(p);
			}
		}

		sys_check_timeouts();  //lwip core timer
		soft_timer_proc();     //user soft timer
		user_loop();           //user code
	}

	return EXIT_SUCCESS;
}


void *slip_out_task_proc(void *par)
{
	SqQueue *Q = (SqQueue *)par;

	APP_DEBUG("slip out task enter success!\n");
	while(slip_out_task_run_flag) {
		int total_len = recv_packet(slip_out_buffer, BUF_SIZE);

#if CHECK_SUM_ON
		int temp_len = total_len;
		total_len = if_api_check(slip_out_buffer, total_len);
		if(total_len < 0) {
			APP_DEBUG("lost %d bytes\n", temp_len);
			continue;
		}
#endif

#ifdef DEBUG
		PRINTF("SLIP R=%d\n", total_len);

		/*
		 * for(int i=0;i<total_len;i++) {
		 * 		printf("%02x ", slip_out_buffer[i]);
		 * }
		 * printf("\n");
		 */
#endif

		struct pbuf* p = pbuf_alloc(PBUF_RAW, total_len, PBUF_POOL);
		if(p != NULL) {
			/* Copy ethernet frame into pbuf */
			pbuf_take(p, slip_out_buffer, total_len);
			/* Put in a queue which is processed in main loop */
			lock_interrupts();
			if(!queue_try_put(Q, p)) {
				/* queue is full -> packet loss */
				pbuf_free(p);
			}
			unlock_interrupts();
		}
	}
	APP_DEBUG("slip out task exit success!\n");
	return NULL;
}

int slip_out_task_start(SqQueue *Q)
{
	slip_out_task_run_flag = 1;
	APP_DEBUG("slip out task start!\n");

	return pthread_create(&gs_slip_out_task_pthread_id, 0, slip_out_task_proc, Q);
}

int slip_out_task_stop(void)
{
    if (slip_out_task_run_flag == 1) {
    	slip_out_task_run_flag = 0;
        pthread_join(gs_slip_out_task_pthread_id, 0);
		//APP_DEBUG("slip out task exit success!\n");
    } else {
        APP_WARN("slip out task failed!\n");
        return -1;
    }
    return 0;
}
