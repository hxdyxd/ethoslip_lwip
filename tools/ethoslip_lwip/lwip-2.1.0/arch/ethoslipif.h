/*
 * ethoslipif.h
 *
 *  Created on: 2018Äê10ÔÂ21ÈÕ
 *      Author: hxdyxd
 */

#ifndef LWIP_2_1_0_ARCH_ETHOSLIPIF_H_
#define LWIP_2_1_0_ARCH_ETHOSLIPIF_H_

#include "slip.h"
#include "lwip/pbuf.h"
#include "lwip/init.h"
#include "lwip/stats.h"
#include "lwip/dhcp.h"
#include "lwip/etharp.h"
#include "lwip/ethip6.h"

#define CHECK_SUM_ON   1

#define ETHERNET_MTU   1400
#define BUF_SIZE       4000

#if CHECK_SUM_ON
//Checksum
int if_api_calculate_checksum(void *buf, unsigned int len);
int if_api_check(void *buf, unsigned int len);
#endif

err_t ethoslipif_low_level_init(struct netif *netif);

#endif /* LWIP_2_1_0_ARCH_ETHOSLIPIF_H_ */
