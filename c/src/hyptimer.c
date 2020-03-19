/* INCLUDES */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>


#include "../libs/hyptimer.h"

/* DEFINES */
#define TIMER_BASE_ADDRESS 0x42800000
//#define TIMER_CONTROLREG_OFFSET
//#define TIMER_DATAREG_OFFSET

#define TIMER_HYPSO_SETTING 0x0
#define TIMER_HYPSO_ENABLE_MASK 0x80
#define TIMER_HYPSO_DISABLE_MASK 0xFF7F
#define TIMER_HYPSO_LOAD_MASK 0x20
#define TIMER_HYPSO_LOAD_VALUE 0x0

static int fd;
static unsigned int *timerPtr;

/* FUNCTIONS */
void hyp_timer_setup()
{
	printf("Timer setup...\r\n");

	printf("\r\nTrying to access memory...\r\n");
	fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (fd == -1)
	{
		perror("Couldn't open /dev/mem");
		exit(0);
	}
	printf("Access grantet. fd:\t%d\r\n", fd);
	printf("Mapping memory...\r\n");
	timerPtr = mmap(NULL, 3*sizeof(unsigned int), PROT_READ|PROT_WRITE, MAP_SHARED, fd, TIMER_BASE_ADDRESS);
	if (timerPtr == MAP_FAILED)
	{
		perror("Couldn't map memory");
		exit(0);
	}
	printf("Mapping complete.\r\n");

	printf("\r\nConfiguring timer...\r\n");
	timerPtr[0] = TIMER_HYPSO_SETTING;
	printf("Timer configured. Loading timer...\r\n");
	timerPtr[1] = TIMER_HYPSO_LOAD_VALUE;
	printf("\r\nTimer setup complete!!\r\n");
}

void hyp_timer_start()
{
	unsigned int temp = timerPtr[0];
	timerPtr[0] = temp | TIMER_HYPSO_LOAD_MASK;
	timerPtr[0] = temp | TIMER_HYPSO_ENABLE_MASK;
}

void hyp_timer_stop()
{
	unsigned int temp = timerPtr[0];
	timerPtr[0] = temp & TIMER_HYPSO_DISABLE_MASK;
}

void hyp_timer_reset()
{
	unsigned int temp = timerPtr[0];
	timerPtr[0] = temp | TIMER_HYPSO_LOAD_MASK;
	timerPtr[0] = temp;
}

unsigned int hyp_timer_getTime()
{
	return timerPtr[2];
}

/*STATIC FUNCTIONS*/
