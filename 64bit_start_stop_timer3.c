#include <stdio.h>
#include "platform.h"
#include "xgpiops.h"
#include "stdbool.h"
#include "xparameters.h"
#include "xstatus.h"
#include "xtmrctr_l.h"
#include "xil_printf.h"


// Defining values for pushbuttons and LED
#define startButton 50  //PB1: pushbutton 1
#define stopButton 51  //PB2: pushbutton 2
#define ledpin 7  // led


// Configuring the GPIO - PS side. There are IO from PL used.
XGpioPs Gpio;
XGpioPs_Config *GPIOConfigPtr;


// Definging values for timers, 0 is timer 1, 1 is timer 2.
#define TIMER_COUNTER_0     0
#define TIMER_COUNTER_1     1


// Defining the timer address
#define TMRCTR_BASEADDR_0     XPAR_TMRCTR_0_BASEADDR


// Definging the mask value for cascade mode
#define XTC_CSR_CASC_MASK		0x00000800

int main(void) {

  int Status;

  GPIOConfigPtr = XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID);

  Status = XGpioPs_CfgInitialize(&Gpio, GPIOConfigPtr, GPIOConfigPtr ->BaseAddr);
  if (Status != XST_SUCCESS) {
      print("GPIO INIT FAILED\r\n");
      return XST_FAILURE;
  }

  XGpioPs_SetDirectionPin(&Gpio, ledpin, 1);
  XGpioPs_SetOutputEnablePin(&Gpio, ledpin, 1);
  XGpioPs_SetDirectionPin(&Gpio, startButton, 0x0);
  XGpioPs_SetDirectionPin(&Gpio, stopButton, 0x0);

  xil_printf("GPIO ready\r\n");

  // Button variables
  bool start;
  bool stop;

  // Timer variables
  u32 ticks_MS = 0;
  u32 ticks_LS = 0;
  u32 ControlStatus;

  long long ticks_start;
  long long ticks_end;

  double time_between;
  bool printstop = true;

  // Clearing the Control Status register.
  // The Control Status register contains a 32 bit that enables different settings.
  // Cascade mode is bit 11.
  //31-12 is reserved.


  // ----------------  Set timer settings --------
  XTmrCtr_SetControlStatusReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_0,0x0); // clear control status reg
  XTmrCtr_WriteReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_0, XTC_TCSR_OFFSET, XTC_CSR_CASC_MASK); // set cascade mode
  //XTmrCtr_SetControlStatusReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_0, XTC_CSR_CASC_MASK);   // these two actually does the same

  xil_printf("Setting up timer\n");


  // ---------------- Check timer settings ----------
  u32 control_value = XTmrCtr_GetControlStatusReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_0);
  xil_printf("Checking timer settings\n");
  xil_printf("Control status register: %d\n", control_value);


  // Writing 880 in hex will enable cascade and timer.
  // writing 800 in hex will disable timer, but keep cascade mode.

  while(1) {

	start = XGpioPs_ReadPin(&Gpio,startButton);
	stop = XGpioPs_ReadPin(&Gpio,stopButton);

	//printf("not cero please: %lu\n", number);

    if(start && printstop == true) {

		ticks_LS = XTmrCtr_GetTimerCounterReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_0);  // Fetch LSB value
		ticks_MS = XTmrCtr_GetTimerCounterReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_1);  // Fetch MSB value

		ticks_start = (long long) ticks_MS << 32 | ticks_LS;
		XTmrCtr_SetLoadReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_0, 0x0);   // Set the value in load register 1
		XTmrCtr_LoadTimerCounterReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_0);  // load the reg 1 value onto counter reg 1

		XTmrCtr_SetLoadReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_1, 0x0); // Set the value in load reg 2
		XTmrCtr_LoadTimerCounterReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_1);
		xil_printf("Timer started\r\n");

		ControlStatus = XTmrCtr_GetControlStatusReg(TMRCTR_BASEADDR_0,TIMER_COUNTER_0);
		XTmrCtr_SetControlStatusReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_0,ControlStatus & (~XTC_CSR_LOAD_MASK));
		XTmrCtr_SetControlStatusReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_1,ControlStatus & (~XTC_CSR_LOAD_MASK));



		XTmrCtr_Enable(TMRCTR_BASEADDR_0, TIMER_COUNTER_0);  // start the timer

    u32 control_value = XTmrCtr_GetControlStatusReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_0);
    xil_printf("Checking timer settings\n");
    xil_printf("Control status register: %d\n", control_value);

		XGpioPs_WritePin(&Gpio, ledpin, 1);
		printstop = false;
    }


    if(stop && printstop == false) {

		XTmrCtr_Disable(TMRCTR_BASEADDR_0, TIMER_COUNTER_0);

    u32 control_value = XTmrCtr_GetControlStatusReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_0);
    xil_printf("Checking timer settings\n");
    xil_printf("Control status register: %d\n", control_value);

		ticks_LS = XTmrCtr_GetTimerCounterReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_0);  // Fetch LSB value
		ticks_MS = XTmrCtr_GetTimerCounterReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_1);  // Fetch MSB value

		ticks_end = (long long) ticks_MS << 32 | ticks_LS;

		xil_printf("Timer stopped\r\n");
		printf("Test!! Ticks_LS: %lu,   Ticks_MS: %lu\r\n",ticks_LS, ticks_MS);
		time_between = ((double)(ticks_end - ticks_start))/100000000.0;
		printf("Time between: %lf seconds\r\n", time_between);
		printstop = true;


		u32 number = XTmrCtr_GetTimerCounterReg(TMRCTR_BASEADDR_0, TIMER_COUNTER_0);
		printf("not cero please: %lu\n", number);
		XGpioPs_WritePin(&Gpio, ledpin, 0);

    }
  }


  return 0;
}
