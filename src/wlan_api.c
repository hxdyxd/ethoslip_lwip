#include <stdio.h>
#include "wifi_constants.h"
#include "wifi_structures.h"
#include "wlan_api.h"
#include "app_debug.h"

#include "lwip_intf.h"
#include "ethernetif.h"
#include "tun.h"

extern rtw_ssid_t gs_ssid;
extern char gs_passwd[100];
extern rtw_security_t gs_security_type;


int wlan_api_pack_proc(unsigned char *buf, unsigned int len)
{
  	struct wlan_api_default_t *api_def = (struct wlan_api_default_t *)buf;
	if(len < sizeof(struct wlan_api_default_t)) {
	  	APP_ERROR("length error\n");
	  	return -1;
	}
	switch(api_def->type) {
		case WLAN_API_TYPE_CONNECT:
		{
		  	if(sizeof(struct wlan_api_wifi_connect_t) != len) {
			  	APP_ERROR("length error\n");
				return -1;
		 	}
			struct wlan_api_wifi_connect_t *api_con =
			  (struct wlan_api_wifi_connect_t *)buf;
			//copy ssid
			if(api_con->ssid_len >= WLAN_API_SSID_MAX_LEN
			  || api_con->password_len >= WLAN_API_PASSWORD_MAX_LEN) {
				APP_ERROR("ssid or passwd length error\n");
				return -1;
			}
			//SAME
			if(memcmp(gs_ssid.val, api_con->ssid, gs_ssid.len) == 0 && 
			   memcmp(gs_passwd, api_con->password, strlen(gs_passwd)) == 0) {
				APP_DEBUG("ssid and passwd was configured\n");
				return 0;
			}
			
			memcpy(gs_ssid.val, api_con->ssid, api_con->ssid_len);
			gs_ssid.val[api_con->ssid_len] = 0;
			gs_ssid.len = api_con->ssid_len;
			APP_WARN("%s\n", gs_ssid.val);
			//copy password
			memcpy(gs_passwd, api_con->password, api_con->password_len);
			gs_passwd[api_con->password_len] = 0;
			APP_WARN("%s\n", gs_passwd);
			//set security type
			if(api_con->password_len == 0) {
			  	gs_security_type = RTW_SECURITY_OPEN;
				APP_WARN("OPEN\n");
			} else {
			  	gs_security_type = RTW_SECURITY_WPA2_AES_PSK;
				APP_WARN("PSK\n");
			}
			
			wlan_connect_wifi();
		  	break;
		}
		case WLAN_API_TYPE_GET_HWADDR:
		  	APP_WARN("%d.%d.%d.%d.%d.%d\n",
					 xnetif[0].hwaddr[0],
					 xnetif[0].hwaddr[1],
					 xnetif[0].hwaddr[2],
					 xnetif[0].hwaddr[3],
					 xnetif[0].hwaddr[4],
					 xnetif[0].hwaddr[5]
			);
		  	memcpy(api_def->src, xnetif[0].hwaddr, xnetif[0].hwaddr_len);
			api_def->type = WLAN_API_TYPE_GET_HWADDR_RET;
			api_def->buf[0] = 0x01;
	  		return sizeof(struct wlan_api_default_t);
		default:
		  	return -1;
	}
	return 0;
}




