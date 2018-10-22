#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "ethoslipif.h"

#include "wlan_api.h"
#include "slip.h"

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

