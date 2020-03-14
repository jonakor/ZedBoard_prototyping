/* INCLUDES */
#include "linux/interrupt.h"

/* DEFINES */
#define IRQ_NUMBER 52

/* FUNCTIONS */
void hyp_interrupt_setup() {

}

void hyp_interrupt_enable() {
	request_irq(IRQ_NUMBER, timestamp_handler, 0, "timestamp", NULL);
}

void hyp_interrupt_disable() {
	free_irq(IRQ_NUMBER, NULL);
}

static irqreturn_t timestamp_handler(int irq, void* dev_id) {
	
}


/*STATIC FUNCTIONS*/
