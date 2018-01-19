/*
    2017 12 31
	@sococos
*/


#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "FreeRTOS.h"
#include "task.h"
#include "http_api.h"
#include "au_fifo.h"
#include "app_debug.h"

#define USE_VS_1003 0
#define USE_MAD_I2S 1
#if USE_VS_1003
	#include "vs1003.h"
#elif USE_MAD_I2S
	#include "mad.h"
	#include "stream.h"
	#include "frame.h"
	#include "synth.h"
	#include "i2s_h.h"
#endif

typedef struct {
  	char *server;
	char *path;
	int port;
} PLAYER_PAR;


void player_start(char *server, char *path, const int port);
void player_stop(void);
uint8_t player_get_status(void);

#endif

