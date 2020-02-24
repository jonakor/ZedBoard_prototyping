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

//Timer
#include "xtmrctr_l.h"


#define BPS_SIGNAL_PIN 50  //BPS signal. Puls per second
#define FLASH_SIGNAL_PIN 51  //Flash signal
#define LED_PIN 7

#define GPIO_DEVICE_ID		XPAR_XGPIOPS_0_DEVICE_ID
#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define GPIO_INTERRUPT_ID	XPAR_XGPIOPS_0_INTR
#define GPIO_BANK	XGPIOPS_BANK0


static void IntrHandler(void *CallBackRef, u32 Bank, u32 Status);
static int SetupInterruptSystem(XScuGic *Intc, XGpioPs *Gpio, u16 GpioIntrId);


static XGpioPs Gpio; /* The Instance of the GPIO Driver */
static XScuGic Intc; /* The Instance of the Interrupt Controller Driver */


static bool BothButtonsPressed = 0;

//XGpioPs Gpio;
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

/*
  bool bpsSignal;
	bool flashSignal;

  bool previousFlash = 0;
  bool previousBPS = 0;
*/
  //Interrupt set up
  Status = SetupInterruptSystem(Intc, Gpio, GPIO_INTERRUPT_ID);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}
  if (Status == XST_SUCCESS) {
    xil_printf("Interrupt setup complete");
  }


	//Loop
	while ( BothButtonsPressed == 0 ) {
    xil_printf("Outside interrupt");
    for (size_t i = 0; i < 1000000000; i++) {
      //Wait for a while
    }
/*
		bpsSignal 	= XGpioPs_ReadPin(&Gpio, BPS_SIGNAL_PIN);
		flashSignal = XGpioPs_ReadPin(&Gpio, FLASH_SIGNAL_PIN);

    XGpioPs_WritePin(&Gpio, LED_PIN, bpsSignal);

    //Check for rising edge
    if (previousFlash != flashSignal) {
      xil_printf("Flash on rising edge!\n", );
    }
    previousFlash = flashSignal;

    if (previousBPS != bpsSignal) {
      xil_printf("BPS on rising edge!\n", );
    }
    previousBPS = bpsSignal;

*/
	}

	return 0;
}


static void IntrHandler(void *CallBackRef, u32 Bank, u32 Status)
{
	XGpioPs *Gpio = (XGpioPs *)CallBackRef;
	bool bpsSignal;
  bool flashSignal;

	/* Push the switch button */
  bpsSignal 	= XGpioPs_ReadPin(&Gpio, BPS_SIGNAL_PIN);
  flashSignal = XGpioPs_ReadPin(&Gpio, FLASH_SIGNAL_PIN);
  XGpioPs_WritePin(Gpio, LED_PIN, bpsSignal);

	if ( (bpsSignal == 1) && (flashSignal == 1) ) {
		//XGpioPs_SetDirectionPin(Gpio, LED_PIN, 1);
		//XGpioPs_SetOutputEnablePin(Gpio, LED_PIN, 1);
		BothButtonsPressed == 1;
	}
}

static int SetupInterruptSystem(XScuGic *GicInstancePtr, XGpioPs *Gpio,
				u16 GpioIntrId)
{
	int Status;

	XScuGic_Config *IntcConfig; /* Instance of the interrupt controller */

	Xil_ExceptionInit();

	/*
	 * Initialize the interrupt controller driver so that it is ready to
	 * use.
	 */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(GicInstancePtr, IntcConfig,
					IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}


	/*
	 * Connect the interrupt controller interrupt handler to the hardware
	 * interrupt handling logic in the processor.
	 */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				(Xil_ExceptionHandler)XScuGic_InterruptHandler,
				GicInstancePtr);

	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = XScuGic_Connect(GicInstancePtr, GpioIntrId,
				(Xil_ExceptionHandler)XGpioPs_IntrHandler,
				(void *)Gpio);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	/* Enable falling edge interrupts for all the pins in bank 0. */
	XGpioPs_SetIntrType(Gpio, GPIO_BANK, 0x00, 0xFFFFFFFF, 0x00);

	/* Set the handler for gpio interrupts. */
	XGpioPs_SetCallbackHandler(Gpio, (void *)Gpio, IntrHandler);


	/* Enable the GPIO interrupts of Bank 0. */
	XGpioPs_IntrEnable(Gpio, GPIO_BANK, (1 << Input_Pin));


	/* Enable the interrupt for the GPIO device. */
	XScuGic_Enable(GicInstancePtr, GpioIntrId);


	/* Enable interrupts in the Processor. */
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

	return XST_SUCCESS;
}
