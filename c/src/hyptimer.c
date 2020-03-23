/* INCLUDES */
//#include <linux/types.h>
//#include <linux/stat.h>
//#include <linux/mman.h>
//#include <linux/fcntl.h>
//#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
//#include <linux/io.h>
#include <uapi/linux/unistd.h>

#include "../libs/hyptimer.h"

/* DEFINES */
#define TIMER_HYPSO_SETTING 0x0000		// Base setting on timer control register
#define TIMER_HYPSO_ENABLE_MASK 0x0080	// (OR) Mask to set timer enale bit
#define TIMER_HYPSO_DISABLE_MASK 0xFF7F	// (AND) Mask to reset timer enable bit
#define TIMER_HYPSO_LOAD_MASK 0x0020	// (OR) Mask to set timer load bit
#define TIMER_HYPSO_LOAD_VALUE 0x0000	// Value to be loaded when load bit is set

/* FUNCTIONS */
void hyp_timer_setup(unsigned int *timerPtr) {
  unsigned int temp;

  printk("\nTimer setup...\n");

  timerPtr[0] = TIMER_HYPSO_SETTING;

  timerPtr[1] = TIMER_HYPSO_LOAD_VALUE;
  temp = timerPtr[0];
  timerPtr[0] = temp | TIMER_HYPSO_LOAD_MASK;
  while (timerPtr[2] != (unsigned int) TIMER_HYPSO_LOAD_VALUE) { // Added to let timer LOAD before continuing
  	ndelay(1); //Delay to prevent system freeze
  }
  timerPtr[0] = temp;

  printk("Timer setup complete!!\n");
}

void hyp_timer_start(unsigned int *timerPtr) {
  unsigned int temp = timerPtr[0];
  timerPtr[0] = temp | TIMER_HYPSO_ENABLE_MASK;
}

void hyp_timer_stop(unsigned int *timerPtr) {
  unsigned int temp = timerPtr[0];
  timerPtr[0] = temp & TIMER_HYPSO_DISABLE_MASK;
}

void hyp_timer_reset(unsigned int *timerPtr) {
  unsigned int temp = timerPtr[0]; // Get current control register
  timerPtr[0] = temp | TIMER_HYPSO_LOAD_MASK; // Set load bit
  while (timerPtr[2] != (unsigned int) TIMER_HYPSO_LOAD_VALUE) { // Added to let timer LOAD before continuing
  	ndelay(1);
  }
  timerPtr[0] = temp; // Reset load bit
}

unsigned int hyp_timer_getTime(unsigned int *timerPtr) { return timerPtr[2]; }

/*STATIC FUNCTIONS*/
