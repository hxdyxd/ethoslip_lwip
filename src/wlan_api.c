#include <stdio.h>
#include "wifi_constants.h"
#include "wifi_structures.h"
#include "wlan_api.h"

extern rtw_ssid_t gs_ssid;
extern char gs_passwd[100];
extern rtw_security_t gs_security_type;


int wlan_api_pack_proc(unsigned char *buf, unsigned int len)
{
  	struct wlan_api_default_t *api_def = (struct wlan_api_default_t *)buf;
	if(len < sizeof(struct wlan_api_default_t)) {
	  	printf("length error\n");
	  	return -1;
	}
	switch(api_def->type) {
		case WLAN_API_TYPE_CONNECT:
		  {
		  	if(sizeof(struct wlan_api_wifi_connect_t) != len) {
			  	printf("length error\n");
				return -1;
		 	}
			struct wlan_api_wifi_connect_t *api_con =
			  (struct wlan_api_wifi_connect_t *)buf;
			//copy ssid
			if(api_con->ssid_len >= WLAN_API_SSID_MAX_LEN
			  || api_con->password_len >= WLAN_API_PASSWORD_MAX_LEN) {
				printf("ssid or passwd length error\n");
				return -1;
			}
			memcpy(gs_ssid.val, api_con->ssid, api_con->ssid_len);
			gs_ssid.val[api_con->ssid_len] = 0;
			gs_ssid.len = api_con->ssid_len;
			printf("%s\n", gs_ssid.val);
			//copy password
			memcpy(gs_passwd, api_con->password, api_con->password_len);
			gs_passwd[api_con->password_len] = 0;
			printf("%s\n", gs_passwd);
			//set security type
			if(api_con->password_len == 0) {
			  	gs_security_type = RTW_SECURITY_OPEN;
				printf("OPEN\n");
			} else {
			  	gs_security_type = RTW_SECURITY_WPA2_AES_PSK;
				printf("PSK\n");
			}
			
			wlan_connect_wifi();
		  	break;
		  }
		default:
		  	return -1;
	}
	return 0;
}
