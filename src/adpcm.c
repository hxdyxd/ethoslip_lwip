#include "adpcm.h"

const int index_adjust[8] = {-1,-1,-1,-1,2,4,6,8};
const int step_table[89] = {7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,
						50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,
						230,253,279,307,337,371,408,449,494,544,598,658,724,796,
						876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,
						2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,
						8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,
						24623,27086,29794,32767};


int adpcm_decode(uint8_t *in_buf, int16_t *out_buff)
{
	int index = 0, cur_sample = 0;
	int i = 4;
	int k = 0;
	int j = 0;
	
	cur_sample = *( (int16_t *)in_buf );
	index = in_buf[2];
	out_buff[k++] = cur_sample;
	
	while (i < 256) {
		for(j = 0; j < 2; j++) {
			int delta;
			uint8_t sb;
			uint8_t code = in_buf[i]>>(j*4);
			if ((code & 8) != 0) sb = 1; else sb = 0;
			code &= 7;
			delta = (step_table[index] * code)/4 + step_table[index]/8;
			if (sb == 1) delta = -delta;
			cur_sample+=delta;
			if (cur_sample>32767) out_buff[k++] = 32767;
			else if (cur_sample<-32768) out_buff[k++] = -32768;
			else out_buff[k++] = cur_sample;
			index += index_adjust[code];
			if (index<0) index = 0;
			if (index>88) index = 88;
		}
		i++;
  	}
  	return k;
}
