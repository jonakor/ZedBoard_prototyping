#ifndef HYPTIMER_H
#define HYPTIMER_H

/* INCLUDES */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
/* DEFINES */
#define TIMER_BASE_ADDRESS 0x42800000

#define AXI_CLOCK_FREQ 100000000
#define AXI_TICKS_PER_SECOND AXI_CLOCK_FREQ
#define AXI_TICKS_PER_MILLIS (AXI_CLOCK_FREQ/1000)
#define AXI_TICKS_PER_MICROS (AXI_CLOCK_FREQ/1000000)

/* FUNCTIONS */

extern void hyp_timer_setup(unsigned int *timerPtr);

extern void hyp_timer_start(unsigned int *timerPtr);

extern void hyp_timer_stop(unsigned int *timerPtr);

extern void hyp_timer_reset(unsigned int *timerPtr);

extern unsigned int hyp_timer_getTime(unsigned int *timerPtr);

#endif //HYPTIMER_H
