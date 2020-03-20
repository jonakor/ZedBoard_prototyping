#include <stdio.h>
#include <stdlib.h>

#include "libs/hyptimer.h"

int main()
{
	unsigned int ticks;

	hyp_timer_setup();
	for (int i = 0; i < 100000000; ++i){ }

	printf("\r\nTimer start\r\n");
	hyp_timer_start();
	for (int i = 0; i < 500000000; ++i){ }

	ticks = hyp_timer_getTime();
	printf("Before timer reset. Ticks: %d\r\n", ticks);
	hyp_timer_reset();
	ticks = hyp_timer_getTime();
	printf("After timer reset. Ticks: %d\r\n", ticks);
	
	for (int i = 0; i < 500000000; ++i){ }
	ticks = hyp_timer_getTime();
	hyp_timer_stop();
	printf("Timer stop. Ticks: %d \r\n", ticks);

	return 0;
}
