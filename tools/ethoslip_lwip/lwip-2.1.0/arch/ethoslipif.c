#include "ethoslipif.h"


static unsigned char lwip_out_buffer[BUF_SIZE];
unsigned char ethoslipif_mac[ETH_HWADDR_LEN] = {0x8e, 0x05, 0xcf, 0x10, 0x23, 0xce};


static err_t ethoslipif_low_level_output(struct netif *netif, struct pbuf *p);


err_t ethoslipif_low_level_init(struct netif *netif)
{
	netif->linkoutput = ethoslipif_low_level_output;
	netif->output     = etharp_output;
	netif->output_ip6 = ethip6_output;
	netif->mtu        = ETHERNET_MTU;
	netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
	//MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, 100000000);
	SMEMCPY(netif->hwaddr, (void *)ethoslipif_mac, ETH_HWADDR_LEN);
	netif->hwaddr_len = ETH_HWADDR_LEN;
	return ERR_OK;
}

static err_t ethoslipif_low_level_output(struct netif *netif, struct pbuf *p)
{
	LINK_STATS_INC(link.xmit);
	/* Update SNMP stats (only if you use SNMP) */
//	MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
//	int unicast = ((p->payload[0] & 0x01) == 0);
//	if (unicast) {
//		MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
//	} else {
//		MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
//	}
//	lock_interrupts();
	int total_len = p->tot_len;
	pbuf_copy_partial(p, lwip_out_buffer, total_len, 0);

	//printf("LWIP R=%d\n", total_len);
#if CHECK_SUM_ON
	total_len = if_api_calculate_checksum(lwip_out_buffer, total_len);
#endif

	send_packet(lwip_out_buffer, total_len);
	/* Start MAC transmit here */
//	unlock_interrupts();
	return ERR_OK;
}




#if CHECK_SUM_ON
int if_api_calculate_checksum(void *buf, unsigned int len)
{
	unsigned char *p = (unsigned char *)buf;
	unsigned int sum = 0;
	unsigned int i;
	for(i=0;i<len;i++,p++)
		sum += *p;
#ifdef DEBUG_CHECKSUM
	printf("sum = %d \r\n", sum);
#endif
	sum = (sum % 65536) ^ 0xffff;
#ifdef DEBUG_CHECKSUM
	printf("rev.hex = 0x%04x \r\n", sum);
#endif
	*p = (sum & 0x7f00) >> 8;
	p++;
	*p = sum & 0x7f;
	return len + 2;
}

int if_api_check(void *buf, unsigned int len)
{
	unsigned char *p = (unsigned char *)buf;
	if(len < 2)
		return -1;
	uint8_t chechsum_low = p[len-1];
	uint8_t chechsum_high = p[len-2];
	p[len-1] = 0;
	p[len-2] = 0;
	if_api_calculate_checksum(p, len-2);

	if(chechsum_low != p[len-1] || chechsum_high != p[len-2])
		return -1;
	return len - 2;
}
#endif




