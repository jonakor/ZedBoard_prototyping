
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>                 // Required for the GPIO functions
#include <linux/interrupt.h>            // Required for the IRQ code
#include <linux/io.h>

#include "libs/hyptimer.h"

//REMEMBER TO REMOVE DEBOUNCE WHEN NECESSARY
#define PPS_SIGNAL_PIN 956    //MIO pin 50 on ZedBoard. Mapped to 906 + mio_pin = 956
#define FLASH_SIGNAL_PIN 957  //MIO pin 51 on ZedBoard. Mapped to 906 + mio_pin = 957

#define INTERRUPT_TRIGGER (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING) //Make interrupts trigger on both edges


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jon & Simen");
MODULE_DESCRIPTION("Module handling interrupts to timestamp edges of pulse");
MODULE_VERSION("1.1");

static unsigned int irqNumberPPS;
static unsigned int irqNumberFLASH;
static unsigned int *timerPtr;

//Interrupt handler. Automatically runs this function on interrupt
static irq_handler_t  timestamp_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point. In this example this
 *  function sets up the GPIOs and the IRQ
 *  @return returns 0 if successful
 */
static int __init ebbgpio_init(void){
  int result = 0;

  printk(KERN_INFO "Initializing timestamp module\n");

  timerPtr = ioremap(TIMER_BASE_ADDRESS, 12);
  hyp_timer_setup(timerPtr);
  hyp_timer_start(timerPtr);

  //-----------------GPIO SETUP------------------------//
  //PPS
  gpio_request(PPS_SIGNAL_PIN, "sysfs");                      // Set up the gpio
  gpio_direction_input(PPS_SIGNAL_PIN);                       // Set the button GPIO to be an input
  gpio_set_debounce(PPS_SIGNAL_PIN, 200);                     // Debounce the button with a delay of 200ms
  gpio_export(PPS_SIGNAL_PIN, false);                         // Causes gpiopin to appear in /sys/class/gpio
  irqNumberPPS = gpio_to_irq(PPS_SIGNAL_PIN);                 // GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us

  //FLASH
  gpio_request(FLASH_SIGNAL_PIN, "sysfs");                    // Set up the gpio
  gpio_direction_input(FLASH_SIGNAL_PIN);                     // Set the button GPIO to be an input
  gpio_set_debounce(FLASH_SIGNAL_PIN, 200);                   // Debounce the button with a delay of 200ms
  gpio_export(FLASH_SIGNAL_PIN, false);                       // Causes gpiopin to appear in /sys/class/gpio
  irqNumberFLASH = gpio_to_irq(FLASH_SIGNAL_PIN);             // GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us


  // This next call requests an interrupt line
  result = request_irq(irqNumberPPS,                          // The interrupt number requested
                      (irq_handler_t) timestamp_irq_handler, // The pointer to the handler function below
                      INTERRUPT_TRIGGER,                     // Interrupt on rising edge (button press, not release)
                      "PPS interrupt",                       // Used in /proc/interrupts to identify the owner
                      NULL);                                 // The *dev_id for shared interrupt lines, NULL is okay


   // This next call requests an interrupt line
  result = request_irq(irqNumberFLASH,                        // The interrupt number requested
                      (irq_handler_t) timestamp_irq_handler, // The pointer to the handler function below
                      INTERRUPT_TRIGGER,                     // Interrupt on rising edge (button press, not release)
                      "FLASH interrupt",                       // Used in /proc/interrupts to identify the owner
                      NULL);                                 // The *dev_id for shared interrupt lines, NULL is okay 

  return result;
}

/** The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required. Used to release the
 *  GPIOs and display cleanup messages.
 */
static void __exit ebbgpio_exit(void){
  hyp_timer_stop(timerPtr);
  iounmap(timerPtr);
  gpio_unexport(PPS_SIGNAL_PIN);                              // Unexport the GPIO
  gpio_unexport(FLASH_SIGNAL_PIN);                            // Unexport the GPIO
  free_irq(irqNumberPPS, NULL);                               // Free the IRQ number, no *dev_id required in this case
  free_irq(irqNumberFLASH, NULL);                             // Free the IRQ number, no *dev_id required in this case
  gpio_free(PPS_SIGNAL_PIN);                                  // Free the GPIO
  gpio_free(FLASH_SIGNAL_PIN);                                // Free the GPIO


  printk(KERN_INFO "Timestamp module exiting!\n");
}

/** @brief The GPIO IRQ Handler function
 *  This function is a custom interrupt handler that is attached to the GPIO above. The same interrupt
 *  handler cannot be invoked concurrently as the interrupt line is masked out until the function is complete.
 *  This function is static as it should not be invoked directly from outside of this file.
 *  @param irq    the IRQ number that is associated with the GPIO -- useful for logging.
 *  @param dev_id the *dev_id that is provided -- can be used to identify which device caused the interrupt
 *  Not used in this example as NULL is passed.
 *  @param regs   h/w specific register values -- only really ever used for debugging.
 *  return returns IRQ_HANDLED if successful -- should return IRQ_NONE otherwise.
 */
static irq_handler_t timestamp_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs){
  u32 ticks;
  u32 micros;
  u32 microsSinceUTC;

  static u32 ppsCount = 0;
  static u32 frameCount = 0;

  static bool prevPpsSignal = 0;
  static bool prevFlashSignal = 0;

  bool ppsSignal  = gpio_get_value(PPS_SIGNAL_PIN);
  bool flashSignal = gpio_get_value(FLASH_SIGNAL_PIN);
  // if (InitFlag) {
  //    prevPpsSignal = 0;
  //    prevFlashSignal = 0;
  //    frameCount = 0;
  //    ppsCount = 0;
  //    InitFlag = 0;
  // }

  //else {
    //PPS signal
    if (ppsSignal != prevPpsSignal) {
       if (ppsSignal == 1){
          hyp_timer_reset(timerPtr);
          ticks = hyp_timer_getTime(timerPtr);
          ppsCount ++;
          printk("\nInterrupt!: Rising edge of PPS. PPS-count: %u\n", ppsCount);
          printk("Timer reset. Ticks: %u\n", ticks);
       }

       else if (ppsSignal == 0) {
          printk("\nInterrupt!: Falling edge of PPS.\n");
       }

    }

  //GET TIME
  ticks = hyp_timer_getTime(timerPtr);
  micros = ticks/AXI_TICKS_PER_MICROS;
  microsSinceUTC = (ppsCount*1000000) + micros;

  //Flash signal
  if (flashSignal != prevFlashSignal) {
    if (flashSignal == 1){
      frameCount++;
      printk("\nInterrupt!: Rising edge of FLASH.\n");
      printk("Frame %4u starts\tUTC-startstamp + %9u us (microseconds)\n", frameCount, microsSinceUTC);


    }

    else if (flashSignal == 0) {
      microsSinceUTC = (ppsCount*1000000) + micros;
      printk("\nInterrupt!: Falling edge of FLASH.\n");
      printk("Frame %4u ends  \tUTC-startstamp + %9u us (microseconds)\n", frameCount, microsSinceUTC);

    }
  }

  prevPpsSignal = ppsSignal;
  prevFlashSignal = flashSignal;

  //}

return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

/// This next calls are  mandatory -- they identify the initialization function
/// and the cleanup function (as above).
module_init(ebbgpio_init);
module_exit(ebbgpio_exit);
