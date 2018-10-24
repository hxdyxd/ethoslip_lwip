#ifndef __WLAN_API_H__
#define __WLAN_API_H__

#include <stdint.h>

#define WLAN_API_TYPE_CONNECT          0x01
#define WLAN_API_TYPE_CONNECT_RET     (WLAN_API_TYPE_CONNECT | 0x80)

#define WLAN_API_TYPE_GET_HWADDR       0x02
#define WLAN_API_TYPE_GET_HWADDR_RET  (WLAN_API_TYPE_GET_HWADDR | 0x80)



#define WLAN_API_SSID_MAX_LEN        (33)
#define WLAN_API_PASSWORD_MAX_LEN    (65)

#define IS_WLAN_API_FRAME_LEN(len)   ((len) > 14)
#define IS_WLAN_API_FRAME(p)         ((p)[12] == 0xff && (p)[13] == 0xff)

#if defined(__IAR_SYSTEMS_ICC__)|| defined (__GNUC__)
#pragma pack(1)
#endif

struct wlan_api_default_t {
  	unsigned char dst[6];
	unsigned char src[6];
	uint16_t ethtype;
  	unsigned char type;
	unsigned char buf[1];
};

struct wlan_api_wifi_connect_t {
  	unsigned char dst[6];
	unsigned char src[6];
	uint16_t ethtype;
  	unsigned char type;
	unsigned char ssid_len;
	unsigned char ssid[WLAN_API_SSID_MAX_LEN];
	unsigned char password_len;
	unsigned char password[WLAN_API_PASSWORD_MAX_LEN];
};

#if defined(__IAR_SYSTEMS_ICC__)|| defined (__GNUC__)
#pragma pack()
#endif

int wlan_api_pack_proc(unsigned char *buf, unsigned int len);

#endif
