// Standard library includes
#include <stdio.h>
#include "platform.h"
#include "stdbool.h"
#include "xparameters.h"
#include "xstatus.h"
#include "xil_printf.h"

// GPIO_PS General InoutOutput
#include "xgpiops.h"

//Timer
#include "xtmrctr_l.h"


#define BPS_SIGNAL_PIN 13  //BPS signal. Puls per second
#define FLASH_SIGNAL_PIN 10  //Flash signal
#define LED_PIN 7

XGpioPs Gpio;
XGpioPs_Config *GPIOConfigPtr;

//Timer
#define TMRCTR_BASEADDR		XPAR_TMRCTR_0_BASEADDR
#define TIMER_COUNTER_0	 0


int main(void) {

  int Status;

	GPIOConfigPtr = XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID);

	Status = XGpioPs_CfgInitialize(&Gpio, GPIOConfigPtr, GPIOConfigPtr ->BaseAddr);
	if (Status != XST_SUCCESS) {
		print("GPIO INIT FAILED\n\r");
		return XST_FAILURE;
	}
	// Define data direction, inputs with 0
	XGpioPs_SetDirectionPin(&Gpio, BPS_SIGNAL_PIN, 0x0);
	XGpioPs_SetDirectionPin(&Gpio, FLASH_SIGNAL_PIN, 0x0);

	// 	Define data direction, inputs with 1
	XGpioPs_SetDirectionPin(&Gpio, LED_PIN, 1);
	XGpioPs_SetOutputEnablePin(&Gpio, LED_PIN, 1);

	bool bpsSignal;
	bool flashSignal;

  bool previousFlash = 0;
  bool previousBPS = 0;

	//Loop
	while(1) {

		bpsSignal 	= XGpioPs_ReadPin(&Gpio, BPS_SIGNAL_PIN);
		flashSignal = XGpioPs_ReadPin(&Gpio, FLASH_SIGNAL_PIN);

    XGpioPs_WritePin(&Gpio, LED_PIN, bpsSignal);

    //Check for rising edge
    if (previousFlash != flashSignal) {
      xil_printf("Flash on rising edge!\n", );
    }
    previousFlash = flashSignal;

    if (previousBPS != bpsSignal) {
      xil_printf("Flash on rising edge!\n", );
    }
    previousFlash = flashSignal;


	}
	return 0;
}
