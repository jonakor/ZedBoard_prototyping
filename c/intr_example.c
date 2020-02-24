// Standard library includes
#include <stdio.h>
#include "platform.h"
#include "stdbool.h"
#include "xparameters.h"
#include "xstatus.h"
#include "xil_printf.h"

// GPIO_PS General InoutOutput
#include "xgpiops.h"

// Interrupt
#include "xscugic.h"
#include "xil_exception.h"
#include "xplatform_info.h"


#define PPS_SIGNAL_PIN 50  //PPS signal pin. Puls per second
#define FLASH_SIGNAL_PIN 51  //Flash signal pin.
#define LED_PIN 7 // Processing system LED pin

#define GPIO_DEVICE_ID		XPAR_XGPIOPS_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define GPIO_INTERRUPT_ID	XPAR_XGPIOPS_0_INTR

#define INTR_TYPE         XGPIOPS_IRQ_TYPE_EDGE_BOTH //Makes interrupt trig on rising edge. Defined in xgpiops.h

XGpioPs Gpio; /* The Instance of the GPIO Driver. Possibly static */
XScuGic my_Gic; /* The Instance of the Interrupt Controller Driver. Possibly static */

XGpioPs_Config *GPIOConfigPtr;
XScuGic_Config *Gic_Config;

static void my_intr_handler(void *CallBackRef); //function called when interrupt occurs.


//loop
int main(void) {
  int status;

  GPIOConfigPtr = XGpioPs_LookupConfig(GPIO_DEVICE_ID);
  status = XGpioPs_CfgInitialize(&Gpio, GPIOConfigPtr, GPIOConfigPtr ->BaseAddr);
  if (status == XST_SUCCESS) {
    xil_printf("GPIO config success!\r\n");
  }
  else {
    xil_printf("GPIO config error...\r\n");

  }

  // Define data direction, inputs with 0
	XGpioPs_SetDirectionPin(&Gpio, PPS_SIGNAL_PIN, 0x0);
	XGpioPs_SetDirectionPin(&Gpio, FLASH_SIGNAL_PIN, 0x0);

	// 	Define data direction, output with 1
	XGpioPs_SetDirectionPin(&Gpio, LED_PIN, 1);
	XGpioPs_SetOutputEnablePin(&Gpio, LED_PIN, 1);
  XGpioPs_WritePin(&Gpio, LED_PIN, 0);

  Xil_ExceptionInit();
  Gic_Config = XScuGic_LookupConfig(INTC_DEVICE_ID);
  status = XScuGic_CfgInitialize(&my_Gic, Gic_Config, Gic_Config->CpuBaseAddress);
  if (status == XST_SUCCESS) {
    xil_printf("GIC config success!!\r\n");
  }
  else {
    xil_printf("GIC config error...\r\n");
  }

  Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler)XScuGic_InterruptHandler, &my_Gic);

  XScuGic_Connect(&my_Gic, GPIO_INTERRUPT_ID,(Xil_ExceptionHandler)my_intr_handler, (void *)&Gpio); //changed (void *)&Gpio to (void)

  XGpioPs_IntrClear(&Gpio, 0, 0x0); //clears out all interrupt enabled by fault
  XGpioPs_IntrClear(&Gpio, 1, 0x0);
  XGpioPs_IntrClear(&Gpio, 2, 0x0);
  XGpioPs_IntrClear(&Gpio, 3, 0x0);

  /* Enable the GPIO interrupts on PPS and Flash signal pin */
  XGpioPs_IntrEnablePin(&Gpio, PPS_SIGNAL_PIN);
  XGpioPs_IntrEnablePin(&Gpio, FLASH_SIGNAL_PIN);
  /* Enable falling edge interrupts for PPS and Flash signal pin*/
  XGpioPs_SetIntrTypePin(&Gpio, PPS_SIGNAL_PIN, INTR_TYPE);
  XGpioPs_SetIntrTypePin(&Gpio, FLASH_SIGNAL_PIN, INTR_TYPE);

  XGpioPs_IntrClearPin(&Gpio, PPS_SIGNAL_PIN); //possible fix to interrupt issue
  XGpioPs_IntrClearPin(&Gpio, FLASH_SIGNAL_PIN);
  XScuGic_Enable(&my_Gic, GPIO_INTERRUPT_ID);

  Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

  xil_printf("Interrupt Initialized\r\n");

  //loop
  while (1) {

    xil_printf("Outside interrupt handler\r\n");

    for (int i = 0; i < 300000000; i++) {  //for-loop to delay prints to console
      //Wait a while
    }
  }

  return 0;
}

static void my_intr_handler(void *CallBackRef){  //function called when interrupt occurs.

  bool ppsSignal;
  bool flashSignal;
  static bool prevPpsSignal = 0;
  static bool prevFlashSignal = 0;

  static u8 count = 0;

  //PPS signal
  ppsSignal 	= XGpioPs_ReadPin(&Gpio, PPS_SIGNAL_PIN); //read PPS signal to verify state
  if (ppsSignal != prevPpsSignal) {
    if (ppsSignal == 1){
      count ++;
      xil_printf("Rising edge of PPS! PPS-count: %i\r\n", count);
      XGpioPs_IntrClearPin(&Gpio, PPS_SIGNAL_PIN);
    }
    else if (ppsSignal == 0) {
      xil_printf("Falling edge of PPS!\r\n");
      XGpioPs_IntrClearPin(&Gpio, PPS_SIGNAL_PIN);
    }
  }

  //Flash signal
  flashSignal = XGpioPs_ReadPin(&Gpio, FLASH_SIGNAL_PIN); //read Flash signal to verify state
  if (flashSignal != prevFlashSignal) {
    if (flashSignal == 1){
      xil_printf("Rising edge of Flash!\r\n");
      XGpioPs_IntrClearPin(&Gpio, FLASH_SIGNAL_PIN);
    }

    else if (flashSignal == 0) {
      xil_printf("Falling edge of Flash!\r\n");
      XGpioPs_IntrClearPin(&Gpio, FLASH_SIGNAL_PIN);
    }
  }

  prevPpsSignal = ppsSignal;
  prevFlashSignal = flashSignal;
  XGpioPs_WritePin(&Gpio, LED_PIN, ppsSignal); // will be handy when interrupt triggers on both edges. led will blink according to PPS-signal

}
