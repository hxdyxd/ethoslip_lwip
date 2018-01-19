#ifndef _APP_DEBUG_H
#define _APP_DEBUG_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define BLUE_FONT "\033[40;34m%s\033[0m "
#define RED_FONT "\033[40;31m%s\033[0m "
#define GREEN_FONT "\033[40;32m%s\033[0m "
#define YELLOW_FONT "\033[40;33m%s\033[0m "
#define PURPLE_FONT "\033[40;35m%s\033[0m "
#define DGREEN_FONT "\033[40;36m%s\033[0m "
#define WHITE_FONT "\033[40;37m%s\033[0m "

extern SemaphoreHandle_t debugSemaphore;

#define TIME_COUNT() xTaskGetTickCount()

#define APP_ERROR(...) xSemaphoreTake( debugSemaphore, portMAX_DELAY );\
					printf("\033[40;32m[%d]\033[0m \033[2;40;33m%s(%d)\033[0m: ",\
					 TIME_COUNT(), __FUNCTION__, __LINE__);\
					printf("\033[1;40;31mERROR\033[0m ");\
                    printf(__VA_ARGS__);\
					xSemaphoreGive( debugSemaphore );
#define APP_WARN(...) xSemaphoreTake( debugSemaphore, portMAX_DELAY );\
					printf("\033[40;32m[%d]\033[0m \033[2;40;33m%s(%d)\033[0m: ",\
					TIME_COUNT(), __FUNCTION__, __LINE__);\
					printf("\033[1;40;33mWARN\033[0m ");\
                    printf(__VA_ARGS__);\
					xSemaphoreGive( debugSemaphore );
#define APP_DEBUG(...) xSemaphoreTake( debugSemaphore, portMAX_DELAY );\
					printf("\033[40;32m[%d]\033[0m \033[2;40;33m%s(%d)\033[0m: ",\
					 TIME_COUNT(), __FUNCTION__, __LINE__);\
                    printf(__VA_ARGS__);\
					xSemaphoreGive( debugSemaphore );
#define PRINTF(...) xSemaphoreTake( debugSemaphore, portMAX_DELAY );\
					printf(__VA_ARGS__);\
					xSemaphoreGive( debugSemaphore );

#endif
