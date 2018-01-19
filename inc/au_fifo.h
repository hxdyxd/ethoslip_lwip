#ifndef _AU_FIFO_H
#define _AU_FIFO_H


#include "stdint.h"
#include "stddef.h"

#define AU_FIFO_LEN (1024*8)

struct au_fifo_t {
	uint16_t start;
	uint16_t end;   //next start
	uint16_t n;   //next n
	uint8_t data[AU_FIFO_LEN];
};

struct au_fifo_t *au_fifo_init();
int au_fifo_get_fill_length(struct au_fifo_t* fifo);
int au_fifo_write(struct au_fifo_t* fifo, const void *buffer, uint16_t len);
int au_fifo_read(struct au_fifo_t* fifo, void *buffer, uint16_t len);

#endif
