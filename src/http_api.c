/*
    2017 12 31
	@sococos
*/


#include "http_api.h"

volatile char tskmad_enable = 1, tskreader_enable = 1;

int getIpForHost(const char *host, struct sockaddr_in *ip)
{
	struct hostent *he;
	struct in_addr **addr_list;
	he = gethostbyname(host);
	if (he == NULL) return 0;
	addr_list = (struct in_addr **) he->h_addr_list;
	if (addr_list[0] == NULL) return 0;
	ip->sin_family = AF_INET;
	memcpy(&ip->sin_addr, addr_list[0], sizeof(ip->sin_addr));
	return 1;
}

//Open a connection to a webserver and request an URL. Yes, this possibly is one of the worst ways to do this,
//but RAM is at a premium here, and this works for most of the cases.
int openConn(const char *streamHost, const char *streamPath, int streamPort)
{
	struct sockaddr_in remote_ip;
	rtl_memset(&remote_ip, 0, sizeof(struct sockaddr_in));
	if (!getIpForHost(streamHost, &remote_ip)) {
		APP_ERROR("GET: Not get IP server <%s>!\n", streamHost);
		return -1;
	}
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		APP_ERROR("GET: Not open socket!\n");
		return -1;
	}

	remote_ip.sin_port = htons(streamPort);
	APP_DEBUG("GET: Connecting to server %s...\n",
			ipaddr_ntoa((const ip_addr_t* )&remote_ip.sin_addr.s_addr));
	if (connect(sock, (struct sockaddr * )(&remote_ip),
			sizeof(struct sockaddr)) != 00) {
		close(sock);
		APP_ERROR("GET: Connect error!\n");
		return -1;
	}
	//Cobble together HTTP request
	write(sock, "GET ", 4);
	write(sock, streamPath, strlen(streamPath));
	write(sock, " HTTP/1.0\r\nHost: ", 17);
	write(sock, streamHost, strlen(streamHost));
	write(sock, "\r\n\r\n", 4);
	//We ignore the headers that the server sends back... it's pretty dirty in general to do that,
	//but it works here because the MP3 decoder skips it because it isn't valid MP3 data.
	return sock;
}

//Open a connection to a webserver and request an URL. Yes, this possibly is one of the worst ways to do this,
//but RAM is at a premium here, and this works for most of the cases.
int openPostConn(const char *streamHost, const char *streamPath, int streamPort, int length)
{
	struct sockaddr_in remote_ip;
	rtl_memset(&remote_ip, 0, sizeof(struct sockaddr_in));
	if (!getIpForHost(streamHost, &remote_ip)) {
		APP_ERROR("POST: Not get IP server <%s>!\n", streamHost);
		return -1;
	}
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
		APP_ERROR("POST: Not open socket!\n");
		return -1;
	}

	remote_ip.sin_port = htons(streamPort);
	APP_DEBUG("POST: Connecting %s...\n",
			ipaddr_ntoa((const ip_addr_t* )&remote_ip.sin_addr.s_addr));
	if (connect(sock, (struct sockaddr * )(&remote_ip),
			sizeof(struct sockaddr)) != 00) {
		close(sock);
		ARR_ERROR("POST: Connect error!\n");
		return -1;
	}
	//Cobble together HTTP request
	write(sock, "POST ", 5);
	write(sock, streamPath, strlen(streamPath));
	write(sock, " HTTP/1.0\r\nHost: ", 17);
	write(sock, streamHost, strlen(streamHost));
	if(1) {
		const char *type_string = "\r\nContent-Type: audio/wav; samplerate=8000";
		write(sock, type_string, strlen(type_string));
	}
	if(1) {
		static char length_string[30];
		sprintf(length_string, "\r\nContent-length: %d", length);
		write(sock, length_string, strlen(length_string));
	}
	write(sock, "\r\n\r\n", 4);
	//We ignore the headers that the server sends back... it's pretty dirty in general to do that,
	//but it works here because the MP3 decoder skips it because it isn't valid MP3 data.
	return sock;
}

int32_t php_strpos(uint8_t *buf, uint32_t buflen, uint8_t *subbuf, uint32_t subbuflen, uint32_t start)
{
	int i;
	for(i = start ; i < buflen - subbuflen; i++) {
		if(memcmp(buf + i, subbuf, subbuflen) == 0) {
			return i;
		}
	}
	return -1;
}


int http_content_len(uint8_t *buf)
{
	int content_len = 0;
	char *content_len_pos = strstr(buf, "Content-Length: ");
	if(content_len_pos) {
		content_len_pos += strlen("Content-Length: ");
		char *content_end_pos = strstr(content_len_pos, "\r\n");
		if(content_end_pos) {
			*(char*)(content_end_pos) = 0;
			return atoi(content_len_pos);
		}
	}
	return 0;
}


int php_file_get_contents(char *host, char *path, uint16_t port, 
						  uint8_t **pbuf, uint8_t **pfree, int bytesLen)
{
  	*pbuf = NULL;
	*pfree = NULL;
	int file_fd = openConn(host, path, port);
	if(file_fd < 0) {
		APP_ERROR("connect failed\n");
		return -1;
	}
	
	int recv_timeout_ms = 3000;
#if defined(LWIP_SO_SNDRCVTIMEO_NONSTANDARD) && (LWIP_SO_SNDRCVTIMEO_NONSTANDARD == 0)	// lwip 1.5.0
	struct timeval recv_timeout;
	recv_timeout.tv_sec = recv_timeout_ms / 1000;
	recv_timeout.tv_usec = recv_timeout_ms % 1000 * 1000;
	setsockopt(file_fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));
#else	// lwip 1.4.1
	setsockopt(file_fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout_ms, sizeof(recv_timeout_ms));
#endif
	
	uint8_t *http_buf = (uint8_t *)malloc(bytesLen);
	if(http_buf == NULL) {
	  	close(file_fd);
		APP_ERROR("malloc failed\n");
		return -1;
	}
	
	int i = 0;
	int n = 0;
	int body_len = 0;
	int content_len = 0;
	int status = HTTP_IDLE;
	int http_body_start = 0;
	int http_status = 0;
	do {
		int r_len = read(file_fd, http_buf + n, bytesLen - n);
		if(r_len <= 0) {
			close(file_fd);
			APP_ERROR("socket read timeout n=%d\n", n);
			free(http_buf);
			http_buf = NULL;
			return -1;
		}
		n += r_len;
		
		switch(status) {
		case HTTP_IDLE:
		  	if(n > 15) {
			  	if(memcmp("200", &http_buf[9], 3) != 0) {
					http_buf[12] = 0;
					APP_ERROR("status == %s\n", &http_buf[9]);
					status = HTTP_ERROR;
			  	}
				http_body_start = php_strpos(http_buf, n, (uint8_t *)"\r\n\r\n", 4, 0);
				
				if(http_body_start > 0) {
					status = HTTP_HEADER;
					i = http_body_start + 4;   //body start
					body_len = n - http_body_start - 4;
					http_buf[http_body_start + 2] = 0;   //find length
					content_len = http_content_len(http_buf);
					APP_DEBUG("content_len = %d body_len = %d\n", content_len, body_len);
					if(content_len <= 0) {
						status = HTTP_ERROR;
					} else if(body_len >= content_len) {
						status = HTTP_FINALLY;
					}
				}
			}
			break;
		case HTTP_HEADER:
		  	body_len += r_len;
			if(body_len >= content_len) {
			  	status = HTTP_FINALLY;
			}
		  	break;
		}
	}while(n < bytesLen && status != HTTP_FINALLY && status != HTTP_ERROR);
	
	close(file_fd);
	
	if(status == HTTP_ERROR) {
	  	APP_ERROR("status == HTTP_ERROR(%d)\n", status);
		free(http_buf);
		http_buf = NULL;
		return -1;
	}
	
	if(status == HTTP_FINALLY) {
		*pbuf = (http_buf + i);
		*pfree = http_buf;
		return body_len;
	}
	
	APP_ERROR("data too long, limit: %d, Content-Length: %d\n", bytesLen, content_len);
	return -1;
}

