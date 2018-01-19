/*
    2017 12 31
	@sococos
*/


#ifndef _HTTP_API_H_
#define _HTTP_API_H_

#include "platform_autoconf.h"
#include "autoconf.h"

#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "osdep_service.h"
#include "device_lock.h"
#include "semphr.h"
#include "queue.h"


#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "dhcp/dhcps.h"
#include "app_debug.h"

#define HTTP_IDLE             0
#define HTTP_HEADER           1
#define HTTP_FINALLY          3
#define HTTP_ERROR            4


int openConn(const char *streamHost, const char *streamPath, int streamPort);
int openPostConn(const char *streamHost, const char *streamPath, int streamPort, int length);
int http_head_read(unsigned char *buf, int len, int ff);
int32_t php_strpos(uint8_t *buf, uint32_t buflen, uint8_t *subbuf, uint32_t subbuflen, uint32_t start);
int php_file_get_contents(char *host, char *path, uint16_t port, uint8_t **pbuf, uint8_t **pfree, int bytesLen);

#endif
