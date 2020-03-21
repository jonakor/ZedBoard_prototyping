/* INCLUDES */
//#include <linux/types.h>
//#include <linux/stat.h>
//#include <linux/mman.h>
//#include <linux/fcntl.h>
//#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
//#include <linux/io.h>
#include <uapi/linux/unistd.h>


#include "../libs/hyptimer.h"

/* DEFINES */

#define TIMER_HYPSO_SETTING 0x0000
#define TIMER_HYPSO_ENABLE_MASK 0x0080
#define TIMER_HYPSO_DISABLE_MASK 0xFF7F
#define TIMER_HYPSO_LOAD_MASK 0x0020
#define TIMER_HYPSO_LOAD_VALUE 0x0000

/* FUNCTIONS */
void hyp_timer_setup(unsigned int *timerPtr)
{
	printk("\nTimer setup...\n");	
	timerPtr[0] = TIMER_HYPSO_SETTING;
	timerPtr[1] = TIMER_HYPSO_LOAD_VALUE;
	printk("\nTimer setup complete!!\n");
}

void hyp_timer_start(unsigned int *timerPtr)
{
	unsigned int temp = timerPtr[0];
	timerPtr[0] = temp | TIMER_HYPSO_LOAD_MASK;
	timerPtr[0] = temp | TIMER_HYPSO_ENABLE_MASK;
}

void hyp_timer_stop(unsigned int *timerPtr)
{
	unsigned int temp = timerPtr[0];
	timerPtr[0] = temp & TIMER_HYPSO_DISABLE_MASK;
}

void hyp_timer_reset(unsigned int *timerPtr)
{
	unsigned int temp = timerPtr[0];
	timerPtr[0] = temp | TIMER_HYPSO_LOAD_MASK;
	timerPtr[0] = temp;
}

unsigned int hyp_timer_getTime(unsigned int *timerPtr)
{
	return timerPtr[2];	
}


/*STATIC FUNCTIONS*/
