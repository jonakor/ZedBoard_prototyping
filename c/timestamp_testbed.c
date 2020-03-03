
#include "timestamp.h"

#define TIMESTAMP_ARRAY_SIZE 2300

int main() {  //test bed



  u32 **timestampArrayPtr = NULL;
  timestampArrayPtr = (u32 **)calloc(TIMESTAMP_ARRAY_SIZE, sizeof(u32 *));
    for (int i = 0; i < TIMESTAMP_ARRAY_SIZE; i++) {
    	timestampArrayPtr[i] = (u32 *)calloc(2, sizeof(u32));
    }

  timestamps_initialize(timestampArrayPtr);

  timestamps_start(timestampArrayPtr);




  //loop
  while (1) {
    //xil_printf("Processing outside interrupt handler... waiting for interrupts\r\n");
//    u32 ticks = XTmrCtr_GetValue(&TimerCounterInst, TIMER_COUNTER_0);
//    u32 micros = ticks/AXI_TICKS_PER_MICROS;

    for (int i = 0; i < 800000000; i++) {  //for-loop to delay prints to console
      //Wait a while
    }
    for (int i = 0; i < TIMESTAMP_ARRAY_SIZE; i++) {
            xil_printf("%9u\t%9u\r\n", timestampArrayPtr[i][0], timestampArrayPtr[i][1]);
    }
    timestamps_stop();
  }


  return 0;
}
