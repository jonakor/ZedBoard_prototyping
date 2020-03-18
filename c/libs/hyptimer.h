#ifndef HYPTIMER_H
#define HYPTIMER_H

/* INCLUDES */
#include "stdlib.h"
/* DEFINES */

/* FUNCTIONS */

void hyp_timer_setup();

void hyp_timer_start();

void hyp_timer_stop();

void hyp_timer_reset();

unsigned int hyp_timer_getTime();

#endif //HYPTIMER_H
