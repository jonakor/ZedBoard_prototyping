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
	for (int i = 0; i < 1000000000; ++i){ }

	ticks = hyp_timer_getTime();
	hyp_timer_stop();
	printf("Timer stop. Ticks: %d \r\n", ticks);

	return 0;
}
