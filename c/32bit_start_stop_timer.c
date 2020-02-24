// GPIO General InoutOutput
#include <stdio.h>
#include "platform.h"
#include "xgpiops.h"
#include "stdbool.h"
#include "xparameters.h"
#include "xstatus.h"
#include "xtmrctr_l.h"
#include "xil_printf.h"

#define startButton 50  //PB1: pushbutton 1
#define stopButton 51  //PB2: pushbutton 2
#define ledpin 7

XGpioPs Gpio;
XGpioPs_Config *GPIOConfigPtr;

#define TMRCTR_BASEADDR_0		XPAR_TMRCTR_0_BASEADDR
#define TIMER_COUNTER_0	 0   // timer 1



int main(void) {

  int Status;

	GPIOConfigPtr = XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID);

	Status = XGpioPs_CfgInitialize(&Gpio, GPIOConfigPtr, GPIOConfigPtr ->BaseAddr);
	if (Status != XST_SUCCESS) {
		print("GPIO INIT FAILED\n\r");
		return XST_FAILURE;
	}

	XGpioPs_SetDirectionPin(&Gpio, ledpin, 1);
	XGpioPs_SetOutputEnablePin(&Gpio, ledpin, 1);
	XGpioPs_SetDirectionPin(&Gpio, startButton, 0x0);
	XGpioPs_SetDirectionPin(&Gpio, stopButton, 0x0);

  bool start;
  bool stop;
  xil_printf("Timer ready!\r\n");


  // Timer
  u32 ControlStatus;
  u32 ticks = 0;
  u32 ticks_start;
  float time_between;
  int printstop = 0;

  XTmrCtr_SetControlStatusReg(TMRCTR_BASEADDR, TIMER_COUNTER_0,0x0);

  XTmrCtr_SetLoadReg(TMRCTR_BASEADDR, TIMER_COUNTER_0, 0x0);
  XTmrCtr_LoadTimerCounterReg(TMRCTR_BASEADDR, TIMER_COUNTER_0);

  ControlStatus = XTmrCtr_GetControlStatusReg(TMRCTR_BASEADDR, TIMER_COUNTER_0);
  XTmrCtr_SetControlStatusReg(TMRCTR_BASEADDR, TIMER_COUNTER_0, ControlStatus & (~XTC_CSR_LOAD_MASK));

  while(1) {
    start = XGpioPs_ReadPin(&Gpio,startButton);
    stop = XGpioPs_ReadPin(&Gpio,stopButton);

    if(start) {
      XTmrCtr_Enable(TMRCTR_BASEADDR, TIMER_COUNTER_0);
      ticks_start = XTmrCtr_GetTimerCounterReg(TMRCTR_BASEADDR, TIMER_COUNTER_0);

      XGpioPs_WritePin(&Gpio, ledpin, 1);
      printstop = 0;
    }
    if(stop) {
      ticks = XTmrCtr_GetTimerCounterReg(TMRCTR_BASEADDR, TIMER_COUNTER_0);
      XTmrCtr_Disable(TMRCTR_BASEADDR, TIMER_COUNTER_0);
      XGpioPs_WritePin(&Gpio, ledpin, 0);

      // Reset ticks on timer
      XTmrCtr_SetLoadReg(TMRCTR_BASEADDR, TIMER_COUNTER_0, 0x0);
      XTmrCtr_LoadTimerCounterReg(TMRCTR_BASEADDR, TIMER_COUNTER_0);

      ControlStatus = XTmrCtr_GetControlStatusReg(TMRCTR_BASEADDR, TIMER_COUNTER_0);
      XTmrCtr_SetControlStatusReg(TMRCTR_BASEADDR, TIMER_COUNTER_0, ControlStatus & (~XTC_CSR_LOAD_MASK));


      // Calculation time between start and stop in seconds
      time_between = ((float)(ticks)-((float)(ticks_start)))/100000000.0;

      if(printstop == 0) {
    	 xil_printf("Ticks: %d\r\n", ticks);
    	 printf("Time between start and stop: %.3f\r\n", time_between); //xil_printf doesn't support %f

    	 printstop = 1;
      }


    }
  }
  return 0;
}
