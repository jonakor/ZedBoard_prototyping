/* INCLUDES */
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "../libs/hyptimer.h"

MODULE_LICENSE("GPL");

/* DEFINES */
#define TIMER_HYPSO_RESET_CONTROLREG_MASK 0xFFFFF000
#define TIMER_HYPSO_SETTING 0x000 // Base setting on timer control register
#define TIMER_HYPSO_ENABLE_MASK 0x080 // (OR) Mask to set timer enale bit
#define TIMER_HYPSO_LOAD_MASK 0x020 // (OR) Mask to set timer load bit
#define TIMER_HYPSO_LOAD_VALUE 0x00000000 // Value to be loaded when load bit is set

/* FUNCTIONS */
void hyp_timer_setup(volatile unsigned int *timerPtr)
{
    timerPtr[0] &= TIMER_HYPSO_RESET_CONTROLREG_MASK; // Reset configurable bits

    timerPtr[0] |= TIMER_HYPSO_SETTING; // Configure timer
    timerPtr[1] = TIMER_HYPSO_LOAD_VALUE; // Set load value

    timerPtr[0] |= TIMER_HYPSO_LOAD_MASK; // Set load bit
    //   while (timerPtr[2] != TIMER_HYPSO_LOAD_VALUE) { // Added to let timer LOAD before continuing
    //      ndelay(1); //Delay to prevent system freeze
    //   }
    timerPtr[0] &= ~TIMER_HYPSO_LOAD_MASK; // Reset load bit
}

void hyp_timer_start(volatile unsigned int *timerPtr)
{
    timerPtr[0] |= TIMER_HYPSO_ENABLE_MASK;
}

void hyp_timer_stop(volatile unsigned int *timerPtr)
{
    timerPtr[0] &= ~TIMER_HYPSO_ENABLE_MASK;
}

void hyp_timer_reset(volatile unsigned int *timerPtr)
{
    timerPtr[0] |= TIMER_HYPSO_LOAD_MASK; // Set load bit
    //   while (timerPtr[2] != TIMER_HYPSO_LOAD_VALUE) { // Added to let timer LOAD before continuing
    //      ndelay(1); //Delay to prevent system freeze
    //   }
    timerPtr[0] &= ~TIMER_HYPSO_LOAD_MASK; // Reset load bit
}

unsigned int hyp_timer_getTime(volatile unsigned int *timerPtr)
{
    return (unsigned int)timerPtr[2];
}
