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
#include "player.h"
#if USE_VS_1003
	#include "vs1003.h"
#elif USE_MAD_I2S
	#include "i2s_h.h"
#endif
#include "http_api.h"
//#include "adpcm.h"
#include "cJSON.h"
#include <ota_8195a.h>

SemaphoreHandle_t debugSemaphore = NULL;
extern void console_init(void);
extern SemaphoreHandle_t xSemaphore;
extern void task_status_timer_init();
extern struct au_fifo_t *audio_fifo, *audio_out_fifo;
extern void showTaskMessage(void);

static TaskHandle_t pvCreatedTask0 = NULL;
static TaskHandle_t pvCreatedPostDataTask0 = NULL;
static TaskHandle_t pvCreateTestTask1 = NULL;
static TaskHandle_t pvCreatedOtaTask = NULL;

static volatile uint8_t post_task_enable = 0;
static volatile uint8_t post_task_finally = 0;
static volatile uint32_t post_task_bytes_size = 0;
static volatile uint8_t *post_task_bytes_buff = NULL;
static volatile uint8_t post_task_run_sflag = 0;

/*
void post_data_task0(void *par)
{
	int fd;
	
	while(post_task_run_sflag == 1) {
	  	fd = openPostConn("cn.w.wyang.top", "/voice.php?file=abc", 80, post_task_bytes_size);
		
		while(post_task_enable == 0) {
		  	if(post_task_run_sflag == 0) {
				close(fd);
				break;
		  	}
		}
		post_task_enable = 0;
		post_task_finally = 0;
		
		write(fd, post_task_bytes_buff, post_task_bytes_size);
		close(fd);
		if(post_task_enable == 1) {
		  	printf("o\n");
		}
		post_task_finally = 1;
	}
	
	vTaskDelete(NULL);
}


void post_task_start(void)
{
  	if(0 == post_task_run_sflag) {
		post_task_run_sflag = 1;
		xTaskCreate(
			post_data_task0, 
			"post 0", 
			2048, 
			NULL, 
			1, 
			&pvCreatedPostDataTask0
		);
	} else {
	  	printf("´íÎó£ºPOST0ÈÎÎñÒÑ¾­ÔÚÔËÐÐ\n");
	}
}

void post_task_stop(void)
{
  	if(1 == post_task_run_sflag) {
	  	post_task_run_sflag = 0;
	} else {
	  	printf("´íÎó£ºPOST0ÈÎÎñÃ»ÓÐÔÚÔËÐÐ\n");
	}
}

#define sign(v) (((v)>0)?1:0)

static uint8_t vad_check(int16_t* data, uint32_t lenght)
{
	int sum=0;
	int delta_sum=0;
	for(int i=0;i<lenght-1;i++){
       	 if(sign(data[i])^sign(data[i+1]))
       	 	delta_sum++;
       	 //printf("%d\n",value[i] );
       	 if(data[i]<0)
       	 	sum-=data[i];
       	 else
       	 	sum+=data[i];
       	//value=(((int16_t)sample_data[i*2])<<8)&sample_data[i*2+1];
    }
	sum >>= 6;
	printf("s:\t%d\t    ds:\t%d", sum, delta_sum);
    if(sum>20000 && delta_sum<150){
    	//gpio_set_level(GPIO_OUTPUT_IO_0,1);
	  	printf(" +\n");
    	return 1;
    }
    else{
	  	printf("\n");
       	//gpio_set_level(GPIO_OUTPUT_IO_0,0);
        return 0;
    }

}
*/

void player_task_proc(void *par)
{
  	//static uint8_t wbuf[2][1024];
	//static int16_t out_buff[505 * 4];
	//static volatile uint8_t flag = 0;
	//uint32_t recv_count = 0;
	int time = 0;
	
#if USE_VS_1003
	VS_HD_Reset();
	VS_Init();
#elif USE_MAD_I2S
	i2s_device_init();
#endif
	
  	while(1) {
	  	//uint8_t status_s = 0;
	  	
	  	//send header
		/*post_task_bytes_buff = (uint8_t *)wav_pcm_header;
		post_task_bytes_size = sizeof(wav_pcm_header);
		post_task_finally = 0;
	  	post_task_enable = 1;
		vTaskResume(&pvCreatedPostDataTask0);
		*/
	  	
		//recv_count = 0;
		
		/*post_task_start();
		
		VS_Soft_Reset();
		printf("Recorder mode... \n");
		recoder_enter_rec_mode(1024 * 4);
		
	  
		do {
		  	int i;
		  	uint8_t *pbuf = (uint8_t *)wbuf[flag];
			
			recoder_read_512bytes(pbuf);
			adpcm_decode(pbuf, out_buff);
			adpcm_decode(pbuf + 256, out_buff + 505);
			 
			recoder_read_512bytes(pbuf + 512);
			adpcm_decode(pbuf + 256*2, out_buff + 505*2);
			adpcm_decode(pbuf + 256*3, out_buff + 505*3);
			
			for(i=0;i<25;i++){
				vad_check(out_buff + 80*i, 80);
			}
			
			post_task_bytes_buff = (uint8_t *)pbuf;
			post_task_bytes_size = 1024;
			post_task_finally = 0;
			post_task_enable = 1;
			
			flag = !flag;
			//printf("[%d] \n", recv_count);
			recv_count++;
		}while(status_s == 1);
		
		post_task_stop();*/
		
		PRINTF("\n\n");
		
		uint8_t *ppbuf;
		uint8_t *ppfree;
		int http_len = php_file_get_contents("cn.w.wyang.top", "/playerdayhot.php", 80, &ppbuf, &ppfree, 2048);
		if(http_len < 0) {
		  	continue;
		}
		
		APP_DEBUG("receive len: %d\n", http_len);
		char *pjson_str = (char *)malloc(http_len + 1);
		if(pjson_str == NULL) {
		  	APP_ERROR("malloc failed\n");
			continue;
		}
		memcpy(pjson_str, ppbuf, http_len);
		pjson_str[http_len] = '\0';
		free(ppfree);
		ppfree = NULL;
		
		cJSON *ocJson = cJSON_Parse(pjson_str);
		if(ocJson == NULL) {
			APP_ERROR("JSON Parse failed\n");
			continue;
		}
		for(int i = 0;i<cJSON_GetArraySize(ocJson);i++) {
			cJSON *ocJson_msg = cJSON_GetArrayItem(ocJson, i);
			cJSON *ocJson_host = cJSON_GetObjectItem(ocJson_msg, "songHostName");
			cJSON *ocJson_path = cJSON_GetObjectItem(ocJson_msg, "songPath");
			
			PRINTF("\n\n");
			
			PRINTF(GREEN_FONT, "----------------Audio Player Message----------------");
			PRINTF("\n");
			APP_DEBUG("Host: %s\n", ocJson_host->valuestring );
			APP_DEBUG("Path: %s\n", ocJson_path->valuestring );
			APP_DEBUG("artistName: %s\n", cJSON_GetObjectItem(ocJson_msg, "artistName")->valuestring );
			APP_DEBUG("songName: %s\n", cJSON_GetObjectItem(ocJson_msg, "songName")->valuestring );
			PRINTF(GREEN_FONT, "----------------Audio Player Message----------------");
			PRINTF("\n");
			
			/* player */
			player_start(ocJson_host->valuestring, ocJson_path->valuestring, 80);
			vTaskDelay(2000);
			time = 0;
			while(1) {
				if(player_get_status() == 0) {
					APP_WARN("mp3 player stop\n");
					break;
				} else {
				  	//xSemaphoreTake( xSemaphore, portMAX_DELAY );
					//APP_DEBUG("MAD: fifo %d %d, %d %d\n", audio_fifo->n, audio_out_fifo->n,
					//					audio_fifo->n-1-audio_fifo->end+audio_fifo->start,
				  	//					audio_out_fifo->n-1-audio_out_fifo->end+audio_out_fifo->start);
					//xSemaphoreGive( xSemaphore );
					//char *pcWriteBuffer[500];
					//vTaskGetRunTimeStats((char *)&pcWriteBuffer);
                    //PRINTF("%s\r\n", pcWriteBuffer);
					//printf("Run... %d s\n", portGET_RUN_TIME_COUNTER_VALUE() );
				}
				vTaskDelay(1000);
			}
			
		}
		
		cJSON_Delete(ocJson);
		free(pjson_str);
		pjson_str = NULL;
		
	}
}

void create_app_main(void)
{
  	APP_DEBUG("Run task %08X\n", (uint32_t)player_task_proc );
	xTaskCreate(	player_task_proc, /* ÈÎÎñº¯Êý */
					"app_main", /* ÈÎÎñÃû */
					512, /* ÈÎÎñÕ»´óÐ¡£¬µ¥Î» word£¬Ò²¾ÍÊÇ 4 ×Ö½Ú */
					NULL, /* ÈÎÎñ²ÎÊý */
					1, /* ÈÎÎñÓÅÏÈ¼¶ */
					&pvCreatedTask0 /* ÈÎÎñ¾ä±ú */
				);
}


void http_update_ota_task_t(void *param)
{
  	while(0) {	
	  	uint8_t *ppbuf = NULL;
		uint8_t *ppfree = NULL;
		int http_len = php_file_get_contents("172.16.21.222", "/ota.php?search", 80, &ppbuf, &ppfree, 1024);
		if(http_len < 5) {
		  	break;
		}
		
		free(ppfree);
		ppfree = NULL;
		APP_WARN("sofeware update start\n");
		
	  	int ret = -1;
		ret = http_update_ota("172.16.21.222", 80, "ota.php?download");

		if(ret) {
			break;
			//PRINTF("\n\r[%s] Ready to run app", __FUNCTION__);
		} else {
			APP_WARN("\n\r[%s] Ready to reboot\n", __FUNCTION__);
			ota_platform_reset();
		}
	}
	APP_WARN("");
	PRINTF(RED_FONT, "OTA: http update task exit! ");
	PRINTF("\n");
	create_app_main();
	vTaskDelete(NULL);
}


void http_update_ota_task_start(void)
{
  	xTaskCreate(	http_update_ota_task_t, /* ÈÎÎñº¯Êý */
					"ota_task", /* ÈÎÎñÃû */
					1024, /* ÈÎÎñÕ»´óÐ¡£¬µ¥Î» word£¬Ò²¾ÍÊÇ 4 ×Ö½Ú */
					NULL, /* ÈÎÎñ²ÎÊý */
					1, /* ÈÎÎñÓÅÏÈ¼¶ */
					&pvCreatedOtaTask /* ÈÎÎñ¾ä±ú */
				);
}


void network_connected_cb()
{
	APP_DEBUG("wifi is connected to ap\n");
	TickType_t tick1 = xTaskGetTickCount();
	vTaskDelay(2000);
	TickType_t tick2 = xTaskGetTickCount();
	PRINTF("\n");
	APP_WARN("Delay 2s: %d\n", tick2 - tick1 );
	
	http_update_ota_task_start();
}

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
void main(void)
{
     /* Initialize log uart and at command service */
	//console_init();
  	debugSemaphore = xSemaphoreCreateMutex();

	/* wlan intialization */
#if defined(CONFIG_WIFI_NORMAL) && defined(CONFIG_NETWORK)
	wlan_network(network_connected_cb);
#endif
	
	//task_status_timer_init();
	
    /*Enable Schedule, Start Kernel*/
#if defined(CONFIG_KERNEL) && !TASK_SCHEDULER_DISABLED
	#ifdef PLATFORM_FREERTOS
	APP_WARN("PLATFORM_FREERTOS %d\n", SystemCoreClock);
	vTaskStartScheduler();
	//printf("FreeRTOS Running...\n");
	#endif
#else
	RtlConsolTaskRom(NULL);
#endif
	
}
