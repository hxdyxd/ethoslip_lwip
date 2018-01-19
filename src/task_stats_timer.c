#include <stdint.h>
#include "timer_api.h"

gtimer_t my_timer1;
volatile uint32_t ulHighFrequencyTimerTicks2 = 0;

void timer1_timeout_handler(uint32_t id)
{
	ulHighFrequencyTimerTicks2++;
}

uint32_t task_status_timer_tick()
{
	return gtimer_read_tick(&my_timer1)>>2;
}

void task_status_timer_reload()
{
	gtimer_reload(&my_timer1, 0);
}

void task_status_timer_init()
{
	// Initial a periodical timer
    gtimer_init(&my_timer1, TIMER0);
    gtimer_start_periodical(&my_timer1, 50, (void*)timer1_timeout_handler, (uint32_t)&my_timer1);
}
