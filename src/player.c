/*
    2017 12 31
	@sococos
*/


#include "player.h"


#if USE_VS_1003
	static TaskHandle_t pvCreatedTask1 = NULL;
	static volatile uint8_t player_task_run_sflag = 0;
#elif USE_MAD_I2S
	static TaskHandle_t pvCreatedMadTask = NULL;
	static volatile uint8_t player_mad_task_run_sflag = 0;
	static volatile uint8_t player_mad_task_stop_sflag = 1;
#endif

static TaskHandle_t pvCreatedPlayerNetTask = NULL;
static volatile uint8_t player_net_task_run_sflag = 0;
static volatile uint8_t player_net_task_stop_sflag = 1;
static PLAYER_PAR player_p;
SemaphoreHandle_t xSemaphore = NULL;
struct au_fifo_t *audio_fifo = NULL, *audio_out_fifo = NULL;
//struct mad_decoder decoder;


void showTaskMessage(void)
{
	const char *state[] = {
		"Running",	/* A task is querying the state of itself, so must be running. */
		"Ready",	/* The task being queried is in a read or pending ready list. */
		"Blocked",	/* The task being queried is in the Blocked state. */
		"Suspended", /* The task being queried is in the Suspended state, 
						or is in the Blocked state with an infinite time out. */
		"Deleted"
	};
	TaskStatus_t *StatusArray;
	UBaseType_t task_num;

	task_num=uxTaskGetNumberOfTasks();      //获取系统任务数量
	APP_DEBUG("RTOS: Number Of Tasks %d\r\n", task_num);
	StatusArray=pvPortMalloc(task_num*sizeof(TaskStatus_t));//申请内存
	if(StatusArray!=NULL)                   //内存申请成功
	{
		uint32_t TotalRunTime;
		int ArraySize = uxTaskGetSystemState((TaskStatus_t*   )StatusArray,   //任务信息存储数组
									   (UBaseType_t     )task_num,  //任务信息存储数组大小
									   (uint32_t*       )&TotalRunTime);//保存系统总的运行时间
		int x;
		for(x=0;x<task_num;x++)
		{
			APP_DEBUG("RTOS: ID:\033[1;33m%d\033[0m P:\033[1;33m%d\033[0m S:\033[1;33m%d\033[0m T:\033[1;33m%d\033[0m %s %s\n",
					StatusArray[x].xTaskNumber,
					StatusArray[x].uxCurrentPriority, //任务优先级
					StatusArray[x].usStackHighWaterMark, //usStackHighWaterMark
					StatusArray[x].ulRunTimeCounter,
					state[StatusArray[x].eCurrentState],
					StatusArray[x].pcTaskName); //任务名称
		}
	}
	vPortFree(StatusArray); //释放内存
}

#if USE_VS_1003
void test_task1(void *par)
{
  	uint8_t wbuf[32];
	uint32_t recv_count = 0;
	
	VS_Soft_Reset();
	VS_Restart_Play();
	VS_Set_All();
	VS_Reset_DecodeTime();
	vs1003_hdl_spi_init(8000000, 16);  //10416666
	
	int flag = 0;
	while(1 == player_task_run_sflag) {
		
		//printf("fifo size: %d\n", au_fifo_get_fill_length(&audio_fifo) );
	  
		if(0 == player_net_task_run_sflag && au_fifo_get_fill_length(audio_fifo) < 32) {
		  	player_task_run_sflag = 0;
		  	break;
		}
		
		int n = au_fifo_read(audio_fifo, &wbuf, 32);
		if(n < 0) {
		  	continue;
		}
		
		int i = 0;
		do {
		  	//printf("%02x \n", *wbuf);
			if( VS_Send_MusicData(wbuf + i) == 0 ) {
				i += 32;
			}
			
		}while(i < n );
	}
	//free(wbuf);
	wbuf = NULL;
	player_task_run_sflag = 0;
	vTaskDelete(NULL);
}

#elif USE_MAD_I2S


#define READBUFSZ (2106)
unsigned char *readBuf;

//Called by the NXP modifications of libmad. Sets the needed output sample rate.
static int oldRate = 0;
void set_dac_sample_rate(int rate, int chls) {
	if (rate == oldRate) return;
	oldRate = rate;
	APP_DEBUG("MAD: Rate %d, channels %d\n", rate, chls);
	i2s_device_set_rate(rate, chls);
}


// Called by the NXP modifications of libmad. It passes us (for the mono synth)
// 32 16-bit samples.
#define DIV_SR_16B(v, b) (((v)&0x8000)?((v)+1)>>(b):(v)>>(b))
#define DIV_16B(v, b) ((v)/(b))

void render_sample_block(short *short_sample_buff, int no_samples)
{
  	static uint16_t ptr[2 * 32];
	int i;
	//printf("render_sample_block\n");
	for(i = 0; i < 32; i++) {
	  	ptr[i<<1] = DIV_16B(short_sample_buff[i], 8);
		ptr[(i<<1) +1] = DIV_16B(short_sample_buff[i+no_samples], 8);
	}
	int r_len;
	do {
		if(audio_out_fifo->n < AU_FIFO_LEN - 64*2) {
		  	/* disable irq */
			__set_PRIMASK(1);
			r_len = au_fifo_write(audio_out_fifo, ptr, 64*2);
			__set_PRIMASK(0);
			if(r_len < 0) {
				vTaskDelay(5);
				//printf("write failed\n");
			}
		} else {
		  	r_len = -1;
			vTaskDelay(5);
		}
	} while(r_len < 0);
	//printf("%d\n", no_samples);
}

//Routine to print out an error
static enum mad_flow error(void *data, struct mad_stream *stream, struct mad_frame *frame)
{
	APP_ERROR("MAD: Dec err 0x%04x (%s)\n", stream->error, mad_stream_errorstr(stream));
	return MAD_FLOW_CONTINUE;
}

enum mad_flow input(void *user_data, struct mad_stream *stream)
{
  	int n, i;
	int rem;//, fifoLen;
	//Shift remaining contents of buf to the front
	rem = stream->bufend - stream->next_frame;
	memmove(readBuf, stream->next_frame, rem);
	
	n = (READBUFSZ - rem); // Calculate amount of bytes we need to fill buffer.
	
	while (rem < READBUFSZ) {
	  
		/*if(audio_out_fifo->n == 0) {
		  	APP_ERROR(" %d %d\n", rem, n);
		}*/
		if(0 == player_net_task_run_sflag) {
		  	if(0 == audio_fifo->n) {
				player_mad_task_run_sflag = 0;
				return MAD_FLOW_STOP;
			} else if(n > audio_fifo->n) {
			  	n = audio_fifo->n;
			}
		}
		
		xSemaphoreTake( xSemaphore, portMAX_DELAY );
		int r_len = au_fifo_read(audio_fifo, &readBuf[rem], n);
		xSemaphoreGive( xSemaphore );
		if(r_len < 0) {
		  	continue;
		}
		
		rem += r_len;
		//printf("rem: %d \n", stream->bufend - stream->next_frame);
		
	}
	//Okay, let MAD decode the buffer.
	mad_stream_buffer(stream, readBuf, READBUFSZ);
	return MAD_FLOW_CONTINUE;
}


/*
enum mad_flow header_func(void *user_data, struct mad_header const *header)
{
  	printf("samplerate: %d channel mode: %d\n", header->samplerate, header->mode);
	return MAD_FLOW_CONTINUE;
}

enum mad_flow output_func(void *user_data, struct mad_header const *header, struct mad_pcm *pcm)
{
  	printf("samplerate: %d channel mode: %d length: %d\n", pcm->samplerate, pcm->channels, pcm->length);
	
  	return MAD_FLOW_CONTINUE;
}
*/


void player_mad_task_proc(void *par)
{
  	player_mad_task_stop_sflag = 0;
  	i2s_device_set_rate(8000, 2);
	
	APP_DEBUG("i2c_device_init\n");
	i2c_device_init();
	APP_DEBUG("wm8978_Reset\n");
	wm8978_Reset();		/* 复位WM8978到复位状态 */
	APP_DEBUG("wm8978_CfgAudioPath\n");
	/* 配置WM8978芯片，输入为DAC，输出为耳机 */
	//wm8978_CfgAudioPath(LINE_ON, EAR_LEFT_ON | EAR_RIGHT_ON);
	wm8978_CfgAudioPath(DAC_ON, EAR_LEFT_ON | EAR_RIGHT_ON); 
	/* 调节音量，左右相同音量 */
	APP_DEBUG("wm8978_SetOUT1Volume\n");
	wm8978_SetOUT1Volume(50);
	
	/* 配置WM8978音频接口为飞利浦标准I2S接口，16bit */
	APP_DEBUG("wm8978_CfgAudioIF\n");
	wm8978_CfgAudioIF(I2S_Standard_Phillips, 16);
	
	
	
	
  	while(au_fifo_get_fill_length(audio_fifo) < AU_FIFO_LEN/2 && player_net_task_run_sflag != 0) {
	  	
		vTaskDelay(10);
  	}
	
  	unsigned char * mad_bufs = malloc( sizeof(struct mad_stream) + sizeof(struct mad_frame)
							+ sizeof(struct mad_synth) + READBUFSZ);
	if (mad_bufs == NULL) {
		player_mad_task_run_sflag = 0;
		APP_ERROR("MAD: Alloc failed\n");
		vTaskDelete(NULL);
	}
	memset(mad_bufs, 0,	sizeof(struct mad_stream) + sizeof(struct mad_frame)
						+ sizeof(struct mad_synth));
	APP_WARN("MAD: Alloc %d bytes at %p\n",
				sizeof(struct mad_stream) + sizeof(struct mad_frame) + sizeof(struct mad_synth) + READBUFSZ,
				mad_bufs);
  	struct mad_stream *stream = (struct mad_stream *)mad_bufs;
	struct mad_frame *frame = (struct mad_frame *)&mad_bufs[sizeof(struct mad_stream)];
	struct mad_synth *synth = (struct mad_synth *)&mad_bufs[sizeof(struct mad_stream) + sizeof(struct mad_frame)];
	readBuf = &mad_bufs[sizeof(struct mad_stream) + sizeof(struct mad_frame)
				+ sizeof(struct mad_synth)];
	
	mad_stream_init(stream);
	mad_frame_init(frame);
	mad_synth_init(synth);
	//i2s_device_start();
	//mad_decoder_init(&decoder, readBuf, input, header_func,
    //             /*filter*/ 0, output_func, error, /* message */ 0);
	//mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
	//mad_decoder_finish(&decoder);
	
	while (player_mad_task_run_sflag == 1) {
	  	xSemaphoreTake( xSemaphore, portMAX_DELAY );
		APP_DEBUG("MAD: fifo %d %d, %d %d\n", audio_fifo->n, audio_out_fifo->n,
							audio_fifo->n-1-audio_fifo->end+audio_fifo->start,
							audio_out_fifo->n-1-audio_out_fifo->end+audio_out_fifo->start);
		xSemaphoreGive( xSemaphore );
	  	input(NULL, stream);
		while (player_mad_task_run_sflag == 1) {
			int r = mad_frame_decode(frame, stream);
			if (r == -1) {
				if (!MAD_RECOVERABLE(stream->error)) {
					//We're most likely out of buffer and need to call input() again
					break;
				}
				error(NULL, stream, frame);
				continue;
			}
			//printf("MAD: Frame synth.\n");
			mad_synth_frame(synth, frame);
			//output_func(NULL, &frame->header, &synth->pcm);
		}
	}
	mad_synth_finish(synth);
	mad_frame_finish(frame);
	mad_stream_finish(stream);
	APP_DEBUG("MAD: Wait I2S transfer finally\n");
	while(audio_out_fifo->n > I2S_DMA_PAGE_SIZE) {
	  	vTaskDelay(100);
	}
	
	free(mad_bufs);
	mad_bufs = NULL;
	free(audio_fifo);
	audio_fifo = NULL;
	free(audio_out_fifo);
	audio_out_fifo = NULL;
	
  	
	//i2s_device_stop();
	showTaskMessage();
	player_mad_task_run_sflag = 0;
	player_net_task_run_sflag = 0;
	APP_WARN("");
	PRINTF(RED_FONT, "MAD: player mad task exit! ");
	PRINTF("\n");
	player_mad_task_stop_sflag = 1;
	vTaskDelete(NULL);
}

#endif


void player_net_server_task(void *par)
{
  	player_net_task_stop_sflag = 0;
  	static uint8_t buffer[768];
	int fd = openConn(player_p.server, player_p.path, player_p.port);
	if(fd < 0) {
		//tskreader_enable = 0;
	  	player_net_task_run_sflag = 0;
		APP_ERROR("openConn failed\n");
		close(fd);
		vTaskDelete(NULL);
	}
	
	int recv_timeout_ms = 6000;
#if defined(LWIP_SO_SNDRCVTIMEO_NONSTANDARD) && (LWIP_SO_SNDRCVTIMEO_NONSTANDARD == 0)	// lwip 1.5.0
	struct timeval recv_timeout;
	recv_timeout.tv_sec = recv_timeout_ms / 1000;
	recv_timeout.tv_usec = recv_timeout_ms % 1000 * 1000;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));
#else	// lwip 1.4.1
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout_ms, sizeof(recv_timeout_ms));
#endif
	
	int n = 0;
	uint8_t *header = (uint8_t *)audio_fifo->data;
	
	do {
		int r_len = read(fd, header + n, 512);
		if(r_len <= 0) {
		  	player_net_task_run_sflag = 0;
			APP_ERROR("socket read timeout 0\n");
		  	break;
		}
		n += r_len;
	}while(n < 2048);
	
	while(n > 0) {
		int start = php_strpos(header, n, (uint8_t *)"\r\n\r\n", 4, 0);
		if(start < 0) {
			player_net_task_run_sflag = 0;
			APP_ERROR("Not find \\0a\\0d\\0a\\0d 0\n");
			break;
		}
		if(1){
			int j;
			APP_DEBUG("HTTP Header: \n");
			for(j = 0; j < start + 4; j++) {
				PRINTF("%c", header[j] );
			}
		}
		
		/* warn: bug bug bug!!!! */
		audio_fifo->start = start + 4;
		audio_fifo->end = n - 1;
		audio_fifo->n = n - start - 4;
		break;
	}
	
  	while(1 == player_net_task_run_sflag) {
		
		//printf("fifo size: %d\n", au_fifo_get_fill_length(audio_fifo) );
		while(audio_fifo->n < AU_FIFO_LEN - sizeof(buffer) ) {
			int r_len = read(fd, buffer, sizeof(buffer) );
			if(r_len <= 0) {
			  	player_net_task_run_sflag = 0;
				APP_WARN("socket read timeout r_len=%d\n", r_len);
				break;
			} else {
				xSemaphoreTake( xSemaphore, portMAX_DELAY );
				au_fifo_write(audio_fifo, buffer, r_len);
				xSemaphoreGive( xSemaphore );
			}
		}
		vTaskDelay(4);
  	}
	
	close(fd);

	showTaskMessage();
	player_net_task_run_sflag = 0;
	APP_WARN("");
	PRINTF(RED_FONT, "player net server task exit!");
	PRINTF("\n");
	player_net_task_stop_sflag = 1;
  	vTaskDelete(NULL);
}

void player_start(char *server, char *path, const int port)
{
	audio_fifo = au_fifo_init();
	if(audio_fifo == NULL) {
		APP_ERROR("Player: Alloc In fifo failed\n");
		return;
	}
	audio_out_fifo = au_fifo_init();
	if(audio_out_fifo == NULL) {
		APP_ERROR("Player: Alloc Out fifo failed\n");
		return;
	}

  	player_net_task_run_sflag = 1;
	player_p.server = server;
	player_p.path = path;
	player_p.port = port;
	xSemaphore = xSemaphoreCreateMutex();
	
	xTaskCreate(
					player_net_server_task,
					"play_net",
					512,
					NULL,
					1,
					&pvCreatedPlayerNetTask
				 );
#if USE_VS_1003
	player_task_run_sflag = 1;
	VS_Soft_Reset();
	VS_Restart_Play();
	VS_Set_All();
	VS_Reset_DecodeTime();
	vs1003_hdl_spi_init(8000000, 16);  //10416666
	
	xTaskCreate(
					test_task1,
					"vs1003",
					1024,
					NULL,
					1,
					&pvCreatedTask1
				 );
#elif USE_MAD_I2S
	player_mad_task_run_sflag = 1;
	
	xTaskCreate(
					player_mad_task_proc,
					"mad_dec",
					2504,
					NULL,
					1,
					&pvCreatedMadTask
				);
	
#endif
}

void player_stop()
{
  	player_net_task_run_sflag = 0;
#if USE_VS_1003
  	player_task_run_sflag = 0;
#elif USE_MAD_I2S
	player_mad_task_run_sflag = 0;
#endif
}

uint8_t player_get_status()
{
#if USE_VS_1003
  	return !player_task_stop_sflag;
#elif USE_MAD_I2S
	return !player_mad_task_stop_sflag;
#else
	return 0;
#endif
}



