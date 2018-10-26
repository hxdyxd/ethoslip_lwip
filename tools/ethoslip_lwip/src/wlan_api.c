#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ethoslipif.h"

#include "wlan_api.h"
#include "slip.h"

static uint8_t gs_wlan_api_status = WLAN_API_TYPE_FREE;
static void (*gs_get_hwaddr_success_callback)(uint8_t *, int) = NULL;

int wlan_api_connect(void *ssid, void *passwd, uint8_t ssid_len, uint8_t passwd_len)
{

	if(ssid_len >= WLAN_API_SSID_MAX_LEN || passwd_len >= WLAN_API_PASSWORD_MAX_LEN) {
		printf("ssid or passwd length error\n");
		return -1;
	}

	struct wlan_api_wifi_connect_t *api_con = NULL;
	api_con	= (struct wlan_api_wifi_connect_t *)malloc(sizeof(struct wlan_api_wifi_connect_t) + 2);
	if(api_con == NULL) {
		printf("mem alloc error\n");
		return -1;
	}
	memset(api_con, 0, sizeof(struct wlan_api_wifi_connect_t) + 2);

	const uint8_t ether_header[] = {
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xff, 0xff
	};
	memcpy(api_con, ether_header, sizeof(ether_header));
	api_con->type = WLAN_API_TYPE_CONNECT;
	memcpy(api_con->ssid, ssid, ssid_len);
	memcpy(api_con->password, passwd, passwd_len);
	api_con->ssid_len = ssid_len;
	api_con->password_len = passwd_len;

	int total_len = sizeof(struct wlan_api_wifi_connect_t);
#if CHECK_SUM_ON
	total_len = if_api_calculate_checksum( (unsigned char *)api_con, total_len);
#endif

	send_packet( (unsigned char *)api_con, total_len);
	free(api_con);
	return 0;
}

int wlan_api_get_hwaddr(void (*success_callback)(uint8_t *, int))
{
	gs_get_hwaddr_success_callback = success_callback;
	struct wlan_api_default_t *api_def = NULL;
	api_def	= (struct wlan_api_default_t *)malloc(sizeof(struct wlan_api_default_t) + 2);
	if(api_def == NULL) {
		printf("mem alloc error\n");
		return -1;
	}

	const uint8_t ether_header[] = {
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0xff, 0xff
	};
	memcpy(api_def, ether_header, sizeof(ether_header));
	api_def->type = WLAN_API_TYPE_GET_HWADDR;
	api_def->buf[0] = 0x00;

	int total_len = sizeof(struct wlan_api_default_t);
#if CHECK_SUM_ON
	total_len = if_api_calculate_checksum( (unsigned char *)api_def, total_len);
#endif

	send_packet( (unsigned char *)api_def, total_len);
	free(api_def);
	gs_wlan_api_status = WLAN_API_TYPE_GET_HWADDR;
	return 0;
}

/*
 * rx proc
 */
int wlan_api_rx_proc(uint8_t *buf, int len)
{
	if(gs_wlan_api_status == WLAN_API_TYPE_FREE) {
		return -1;
	} else if(!IS_WLAN_API_FRAME_LEN(len)) {
		return -1;
	} else if(!IS_WLAN_API_FRAME(buf)) {
		return -1;
	}

	struct wlan_api_default_t *api_def = (struct wlan_api_default_t *)buf;

	if(api_def->type == WLAN_API_TYPE_GET_HWADDR_RET && gs_wlan_api_status == WLAN_API_TYPE_GET_HWADDR) {
		if(gs_get_hwaddr_success_callback != NULL) {
			gs_get_hwaddr_success_callback(api_def->src, 6);
			gs_wlan_api_status = WLAN_API_TYPE_FREE;
		}
	}

	return 0;
}

