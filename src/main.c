/*
    2017 12 31
	@sococos
*/

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "diag.h"
#include "main.h"
#include "wifi_conf.h"

#include "app_debug.h"
#include "tun.h"
#include "time.h"
#include "uart2.h"

SemaphoreHandle_t debugSemaphore = NULL;
static TaskHandle_t net_task_handle = NULL;
extern u8 airkiss_connection_done;

uint8_t network_init_status(void);
void wlan_connect_wifi(void);
int airkiss_start(rtw_network_info_t *wifi);


void network_connected_proc(void *par)
{
  	while( !network_init_status() );
	APP_DEBUG("wifi is connected to ap\n");
	slip2ip_task_start();
	
	//char *ssid_2 = "GXNU.LOCK";
	//char *passwd_2 = "12345678";
	//wifi_start_ap(ssid_2, RTW_SECURITY_WPA_AES_PSK, passwd_2, strlen(ssid_2), strlen(passwd_2), 6);
	
	char *ssid_2 = "GXNU-OPEN";
	wifi_start_ap(ssid_2, RTW_SECURITY_OPEN, NULL, strlen(ssid_2), 0, 6);
	
	tun_task_start();  //tun service start
	///**********************************************************
	while(1) {
	  	if(wifi_is_connected_to_ap() != RTW_SUCCESS) {
		  	APP_WARN("wifi ap connection error, retry .\n");
			wlan_connect_wifi();
			PRINTF("\n");
	  	}
		vTaskDelay(2000);
	}
	//*********************************************************/
}


void net_task_start(void)
{
  	APP_DEBUG("Run net %08X\n", (uint32_t)network_connected_proc );
	xTaskCreate(network_connected_proc,
				"net_task",
				1024,
				NULL,
				1,
				&net_task_handle
	);
}
 
/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
void main(void)
{
    /* Initialize log uart and at command service */
  	debugSemaphore = xSemaphoreCreateMutex();
	/* wlan intialization */
//#if defined(CONFIG_WIFI_NORMAL) && defined(CONFIG_NETWORK)
	uart2_init();
	wlan_network();
//#endif
	net_task_start();
	
    /*Enable Schedule, Start Kernel*/
#if defined(CONFIG_KERNEL) && !TASK_SCHEDULER_DISABLED
	#ifdef PLATFORM_FREERTOS
	APP_WARN("PLATFORM FREERTOS %d\n", SystemCoreClock);
	vTaskStartScheduler();
	#endif
#else
	RtlConsolTaskRom(NULL);
#endif
}
