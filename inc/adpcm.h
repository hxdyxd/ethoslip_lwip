#ifndef _ADPCM_H_
#define _ADPCM_H_


#include <stdint.h>

/* in 256, out 505*2 */
int adpcm_decode(uint8_t *in_buf, int16_t *out_buff);

#endif
