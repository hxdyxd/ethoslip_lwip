#include <time.h>

unsigned int sys_now(void)
{
	return clock()/(CLOCKS_PER_SEC/1000);
}
