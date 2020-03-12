/* INCLUDES */
#include <sys/types.h>
//??#include <sys/stat.h>
#include <fcntl.h>

#include "hyptimer.h"

/* DEFINES */
#define TIMER_BASE_ADDRESS
#define TIMER_CONTROLREG_OFFSET
#define TIMER_DATAREG_OFFSET

#define TIMER_HYPSO_SETTING 0x0
#define TIMER_HYPSO_ENABLE_MASK 0x80
#define TIMER_HYPSO_DISABLE_MASK 0xFF7F
#define TIMER_HYPSO_LOAD_MASK 0x20
#define TIMER_HYPSO_LOAD_VALUE 0x0

static int fd = open("/dev/mem", O_RDWR|O_SYNC);
static u32 *timerPtr = mmap(0, 3, PROT_READ|PROT_WRITE, MAP_SHARED, fd, TIMER_BASE_ADDRESS);

/* FUNCTIONS */
void hyp_timer_setup() {
	timerPtr[0] = TIMER_HYPSO_SETTING;
	timerPtr[1] = TIMER_HYPSO_LOAD_VALUE;
}

void hyp_timer_start() {
	u32 temp = timerPtr[0];
	timerPtr[0] = temp | TIMER_HYPSO_LOAD_MASK;
	timerPtr[0] = temp | TIMER_HYPSO_ENABLE_MASK;
}

void hyp_timer_stop() {
	u32 temp = timerPtr[0];
	timerPtr[0] = temp & TIMER_HYPSO_DISABLE_MASK;
}

void hyp_timer_reset() {
	u32 temp = timerPtr[0];
	timerPtr[0] = temp | TIMER_HYPSO_LOAD_MASK;
	timerPtr[0] = temp;
}

u32 hyp_timer_getTime() {
	return timerPtr[2];
}

/*STATIC FUNCTIONS*/
