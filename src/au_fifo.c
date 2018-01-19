#include "au_fifo.h"

/* AUDIO FIFO */

struct au_fifo_t *au_fifo_init()
{
  	struct au_fifo_t *fifo = (struct au_fifo_t *)malloc(sizeof(struct au_fifo_t));
	if(fifo == NULL) {
	  	return NULL;
	} else {
		fifo->start = 0;
		fifo->end = AU_FIFO_LEN-1;
		fifo->n = 0;
		return fifo;
	}
}

int au_fifo_get_fill_length(struct au_fifo_t* fifo)
{
  	return fifo->n;
}

/* len pre 32 bit */
int au_fifo_write(struct au_fifo_t* fifo, const void *buffer, uint16_t len)
{
  	if(AU_FIFO_LEN - fifo->n < len) {
		return -1;
	} else if(len + fifo->end < AU_FIFO_LEN) {
		memcpy(&fifo->data[fifo->end + 1], buffer, len);
		fifo->end += len;
	} else {
	  	uint32_t write_size = AU_FIFO_LEN - fifo->end - 1;
	  	memcpy(&fifo->data[fifo->end + 1], buffer, write_size );
		memcpy(fifo->data, (uint8_t *)buffer + write_size , len - write_size );
		fifo->end = len - write_size - 1;
	}
	fifo->n += len;
	//printf("[w] start:\t%d \tend:\t%d \tlen:\t%d\n", fifo->start, fifo->end, fifo->n );
	return len;
}

int au_fifo_read(struct au_fifo_t* fifo, void *buffer, uint16_t len)
{
  	if(fifo->n < len) {
		return -1;
	} else if(fifo->start + len <= AU_FIFO_LEN) {
	  	memcpy(buffer, &fifo->data[fifo->start], len);
		fifo->start += len;
		if(fifo->start >= AU_FIFO_LEN) {
		  	fifo->start = 0;
		}
	} else {
	  	uint32_t read_size = AU_FIFO_LEN - fifo->start;
	  	memcpy(buffer, &fifo->data[fifo->start], read_size);
		memcpy( (uint8_t *)buffer + read_size, fifo->data, len - read_size );
		fifo->start = (len - read_size);
	}
	fifo->n -= len;
	//printf("[r] start:\t%d \tend:\t%d \tlen:\t%d\n", fifo->start, fifo->end, au_fifo_get_fill_length(fifo) );
	return len;
}

/* AUDIO FIFO END */