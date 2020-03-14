//////////////////////////////////////////////////////////
//              REMEMBER HEAP-MEMORY SIZE               //
//////////////////////////////////////////////////////////

// Standard library includes
#include <stdio.h>
#include <stdbool.h>
#include <xparameters.h>
#include <xstatus.h>
#include <xil_printf.h>

//Timer
// #include "xtmrctr_l.h"
#include <xtmrctr.h>

// GPIO_PS General InoutOutput
#include <xgpiops.h>

// Interrupt
#include <xscugic.h>
#include <xil_exception.h>
#include <xplatform_info.h>

#include "timestamp.h"

#define TIMER_COUNTER_0     0
#define TMRCTR_BASEADDR_0     XPAR_TMRCTR_0_BASEADDR // Defining the timer address

//Clockfrequency
#define AXI_CLOCK_FREQ XPAR_AXI_TIMER_0_CLOCK_FREQ_HZ
#define AXI_TICKS_PER_SECOND AXI_CLOCK_FREQ
#define AXI_TICKS_PER_MILLIS (AXI_CLOCK_FREQ/1000)
#define AXI_TICKS_PER_MICROS (AXI_CLOCK_FREQ/1000000)
#define TIMESTAMP_ARRAY_SIZE 2300       // Check what size is recommended. How many frames will there be?

static XTmrCtr TimerCounterInst;
//static u32 timestampArray[TIMESTAMP_ARRAY_SIZE][2];

#define GPIO_DEVICE_ID		XPAR_XGPIOPS_0_DEVICE_ID
#define PPS_SIGNAL_PIN 50//13  //PPS signal pin. Puls per second
#define FLASH_SIGNAL_PIN 51//12  //Flash signal pin.
#define LED_PIN 7 // Processing system LED pin

static XGpioPs Gpio; /* The Instance of the GPIO Driver. Possibly static */
static XGpioPs_Config *GPIOConfigPtr;

#define INTC_DEVICE_ID		XPAR_SCUGIC_SINGLE_DEVICE_ID
#define GPIO_INTERRUPT_ID	XPAR_XGPIOPS_0_INTR
#define INTR_TYPE         XGPIOPS_IRQ_TYPE_EDGE_BOTH //Makes interrupt trig on rising edge. Defined in xgpiops.h

static XScuGic my_Gic; /* The Instance of the Interrupt Controller Driver. Possibly static */
static XScuGic_Config *Gic_Config;
static bool InitFlag = 0;


static void my_intr_handler(u32 **arrayPointer); //function called when interrupt occurs.  *CallBackRef
static void gpio_setup();
static void timer_setup();
static void interrupt_setup(u32 **arrayPointer);


// -------------- FUNCTIONS ---------------------

void timestamps_initialize(u32 **arrayPointer) {
  for (int i = 0; i < TIMESTAMP_ARRAY_SIZE; i++) {
    for (int j = 0; j < 2; j++) {
      arrayPointer[i][j] = 0;
    }
  }

  gpio_setup();
  timer_setup();
  interrupt_setup(arrayPointer);
}
void timestamps_start(u32 **arrayPointer) {
  XTmrCtr_Start(&TimerCounterInst, TIMER_COUNTER_0); //Starts timer

  InitFlag = 1;
  my_intr_handler(arrayPointer); //Reseting frame and PPS countervalues to zero

  XScuGic_Enable(&my_Gic, GPIO_INTERRUPT_ID); // enables interrupts
  Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ); // enables interrupts
  xil_printf("Timer started and interrupts enabled\r\n");
}
void timestamps_stop() {
  XScuGic_Disable(&my_Gic, GPIO_INTERRUPT_ID);
  XTmrCtr_Stop(&TimerCounterInst, TIMER_COUNTER_0); //Starts timer
  XTmrCtr_Reset(&TimerCounterInst, TIMER_COUNTER_0);
}



static void my_intr_handler(u32 **arrayPointer) {  //function called when interrupt occurs. *CallBackRef

  u32 ticks;
  u32 micros;
  u32 microsSinceUTC;

  static u32 ppsCount = 0;
  static u32 frameCount = 0;

  static bool prevPpsSignal = 0;
  static bool prevFlashSignal = 0;

  bool ppsSignal  = XGpioPs_ReadPin(&Gpio, PPS_SIGNAL_PIN); //read PPS signal to verify state
  bool flashSignal = XGpioPs_ReadPin(&Gpio, FLASH_SIGNAL_PIN); //read Flash signal to verify state
  if (InitFlag) {
    prevPpsSignal = 0;
    prevFlashSignal = 0;
    frameCount = 0;
    ppsCount = 0;
    InitFlag = 0;
  }

  else {
    //PPS signal
    if (ppsSignal != prevPpsSignal) {
      if (ppsSignal == 1){
        XTmrCtr_Reset(&TimerCounterInst, TIMER_COUNTER_0);
        ppsCount ++;
        xil_printf("Rising edge of PPS! PPS-count: %u\r\n", ppsCount);
      }

      else if (ppsSignal == 0) {
        //xil_printf("Falling edge of PPS!\r\n");
      }

      XGpioPs_IntrClearPin(&Gpio, PPS_SIGNAL_PIN);
    }

    //GET TIME
    ticks = XTmrCtr_GetValue(&TimerCounterInst, TIMER_COUNTER_0);
    micros = ticks/AXI_TICKS_PER_MICROS;
    microsSinceUTC = (ppsCount*1000000) + micros;

    //Flash signal
    if (flashSignal != prevFlashSignal) {
      if (flashSignal == 1){
        frameCount++;
        //xil_printf("Rising edge of flash\r\n");
        arrayPointer[frameCount-1][0] = microsSinceUTC;
        xil_printf("Frame %4u starts\tUTC-startstamp + %9u us (microseconds)\r\n", frameCount, arrayPointer[frameCount-1][0]);


      }

      else if (flashSignal == 0) {
        microsSinceUTC = (ppsCount*1000000) + micros;
        //xil_printf("Falling edge of Flash\r\n");
        arrayPointer[frameCount-1][1] = microsSinceUTC;
        xil_printf("Frame %4u ends  \tUTC-startstamp + %9u us (microseconds)\r\n", frameCount, arrayPointer[frameCount-1][1]);

      }
      XGpioPs_IntrClearPin(&Gpio, FLASH_SIGNAL_PIN);
    }

    prevPpsSignal = ppsSignal;
    prevFlashSignal = flashSignal;
    XGpioPs_WritePin(&Gpio, LED_PIN, ppsSignal); // will be handy when interrupt triggers on both edges. led will blink according to PPS-signal

  }
}
static void gpio_setup() {
  u32 status;

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

  //  Define data direction, output with 1
  XGpioPs_SetDirectionPin(&Gpio, LED_PIN, 1);
  XGpioPs_SetOutputEnablePin(&Gpio, LED_PIN, 1);
  XGpioPs_WritePin(&Gpio, LED_PIN, 0);
}
static void interrupt_setup(u32 **arrayPointer) {
  u32 status;

  Xil_ExceptionInit();
  Gic_Config = XScuGic_LookupConfig(INTC_DEVICE_ID);
  status = XScuGic_CfgInitialize(&my_Gic, Gic_Config, Gic_Config->CpuBaseAddress);
  if (status != XST_SUCCESS) {
    xil_printf("GIC config error...\r\n");
  }

  Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,(Xil_ExceptionHandler)XScuGic_InterruptHandler, &my_Gic);

  XScuGic_Connect(&my_Gic, GPIO_INTERRUPT_ID,(Xil_ExceptionHandler)my_intr_handler, (u32 **)arrayPointer); //changed (void *)&Gpio to (void)

  // XGpioPs_IntrClear(&Gpio, 0, 0x0); //clears out all interrupt enabled by fault
  // XGpioPs_IntrClear(&Gpio, 1, 0x0);
  // XGpioPs_IntrClear(&Gpio, 2, 0x0);
  // XGpioPs_IntrClear(&Gpio, 3, 0x0);

  /* Enable the GPIO interrupts on PPS and Flash signal pin */
  XGpioPs_IntrEnablePin(&Gpio, PPS_SIGNAL_PIN);
  XGpioPs_IntrEnablePin(&Gpio, FLASH_SIGNAL_PIN);
  /* Enable rising and falling edge interrupts for PPS and Flash signal pin*/
  XGpioPs_SetIntrTypePin(&Gpio, PPS_SIGNAL_PIN, INTR_TYPE);
  XGpioPs_SetIntrTypePin(&Gpio, FLASH_SIGNAL_PIN, INTR_TYPE);

  XGpioPs_IntrClearPin(&Gpio, PPS_SIGNAL_PIN);
  XGpioPs_IntrClearPin(&Gpio, FLASH_SIGNAL_PIN);


  xil_printf("Interrupt Initialized\r\n");
}
static void timer_setup() {
  XTmrCtr_Initialize(&TimerCounterInst,TIMER_COUNTER_0);
  XTmrCtr_SetResetValue(&TimerCounterInst, TIMER_COUNTER_0, 0);
  xil_printf("Timer setup complete\r\n");
}
