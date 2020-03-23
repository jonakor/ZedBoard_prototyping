
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/gpio.h>  // Required for the GPIO functions
#include <linux/init.h>
#include <linux/interrupt.h>  // Required for the IRQ code
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>  // Required for memory allocation
#include <linux/uaccess.h>

#include "libs/hyptimer.h"

//REMEMBER TO REMOVE DEBOUNCE WHEN NECESSARY
#define PPS_SIGNAL_PIN 956    //MIO pin 50 on ZedBoard. Mapped to 906 + mio_pin = 956
#define FLASH_SIGNAL_PIN 957  //MIO pin 51 on ZedBoard. Mapped to 906 + mio_pin = 957

#define INTERRUPT_TRIGGER (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING) //Make interrupts trigger on both edges

#define N_FRAME_MAX 2300
#define TIMESTAMP_ARRAY_SIZE (N_FRAME_MAX * 2 * 4) // Two timestamps per frame, four bytes per timestamp.

#define microsSinceUTC( nPPS, ticks) ((nPPS*1000000) + (ticks/AXI_TICKS_PER_MICROS))


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jon & Simen");
MODULE_DESCRIPTION("Module handling interrupts to timestamp edges of pulse");
MODULE_VERSION("2");

static unsigned int irqNumberPPS;
static unsigned int irqNumberFLASH;
static unsigned int *timerPtr;
static unsigned int *tstampArrayPtr;

// Custom device driver
static struct cdev custom_dev;
static char* device_name = "timestamps";
static dev_t device_number;
static int tstamp_open(struct inode *inode, struct file *filp);
static ssize_t tstamp_read(struct file *filp, char __user *buff, size_t count, loff_t *offp);
static struct class *cl;

//Interrupt handler. Automatically runs this function on interrupt
static irq_handler_t  timestamp_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);

static int timestamp_gpio_setup(void); // GPIO and interrupt setup function
static void timestamp_gpio_close(void); // GPIO and interrupt close function

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point. In this example this
 *  function sets up the GPIOs and the IRQ
 *  @return returns 0 if successful
 */
static int __init timestamp_init(void){
  int result = 0;

  //Initialize structure for file operations
  static struct file_operations tstamp_fops = {
    .owner= THIS_MODULE,
    .open = tstamp_open,
    .read = tstamp_read
  };

  //Initialize Driver By Getting Device Number
  if (alloc_chrdev_region(&device_number, 0, 1, device_name) != 0) {
    printk("Can't Allocate Device Number\n");
    return -1;
  }
  //Initialize Custom Device
  cdev_init(&custom_dev, &tstamp_fops);
  //Create Class and Device
  cl = class_create(THIS_MODULE, device_name);
  device_create(cl, NULL, device_number, NULL, device_name);
  //Add the device to the system
  cdev_add(&custom_dev, device_number, 1);

  // Gpio and interrupt setup. Returns 0 or error code
  result = timestamp_gpio_setup();
  if (result < 0) return result;
  
  // Map physical address of timer to pointer
  timerPtr = ioremap(TIMER_BASE_ADDRESS, 12);
  if (timerPtr == NULL) {
    printk("Couldn't map physical adress %x", TIMER_BASE_ADDRESS);
    return -1;
  }
  hyp_timer_setup(timerPtr);
  hyp_timer_start(timerPtr);

  // Allocate timestamp array
  tstampArrayPtr = kzalloc(TIMESTAMP_ARRAY_SIZE, GFP_KERNEL);

  

  printk("Timestamp module loaded.");
  return 0;
}

/** The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required. Used to release the
 *  GPIOs and display cleanup messages.
 */
static void __exit timestamp_exit(void) {
  kfree(tstampArrayPtr);
  hyp_timer_stop(timerPtr);
  iounmap(timerPtr);

  timestamp_gpio_close();

  //Destory Device and Class
  device_destroy(cl, device_number);
  class_destroy(cl);  
  //Delete Custom Device from System
  cdev_del(&custom_dev);
  //Unregister Current Device Name
  unregister_chrdev_region(device_number, 1);


  printk("Timestamp module unloaded.\n");
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
  unsigned int ticks;

  static unsigned int ppsCount = 0;
  static unsigned int frameCount = 0;
  static unsigned int prevFrameCount = 0;

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
          if (frameCount > 0) ppsCount ++;
          printk("\nInterrupt!: Rising edge of PPS. PPS-count: %u\n", ppsCount);
       }
       else if (ppsSignal == 0) {
          printk("\nInterrupt!: Falling edge of PPS.\n");
          if ( (frameCount == prevFrameCount) && (frameCount != 0) ) {
            frameCount = 0;
            ppsCount = 0;

          }
          prevFrameCount = frameCount;
       }
    }

  //Get ticks since the last PPS
  ticks = hyp_timer_getTime(timerPtr);

  //Flash signal
  if (flashSignal != prevFlashSignal) {
    if (flashSignal == 1) {
      tstampArrayPtr[frameCount*2] = microsSinceUTC(ppsCount, ticks);
      printk("\nInterrupt!: Rising edge of FLASH.\n");
      printk("Frame %4u starts\tUTC-startstamp + %9u us (microseconds)\n", frameCount, microsSinceUTC(ppsCount, ticks));      
    }

    else if (flashSignal == 0) {
      tstampArrayPtr[frameCount*2 + 1] = microsSinceUTC(ppsCount, ticks);
      printk("\nInterrupt!: Falling edge of FLASH.\n");
      printk("Frame %4u ends  \tUTC-startstamp + %9u us (microseconds)\n", frameCount, microsSinceUTC(ppsCount, ticks));
      frameCount++;
    }
  }

  prevPpsSignal = ppsSignal;
  prevFlashSignal = flashSignal;

  //}

return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

// Gpio setup.
static int timestamp_gpio_setup(void) {
  int result;
  //    PPS     //
  result = gpio_request(PPS_SIGNAL_PIN, "PPS"); // Set up the gpio
  if (result < 0) return result;

  result = gpio_direction_input(PPS_SIGNAL_PIN);  // Set the button GPIO to be an input
  if (result < 0) return result;
  gpio_set_debounce(PPS_SIGNAL_PIN, 200); // Debounce the button with a delay of 200ms
  gpio_export(PPS_SIGNAL_PIN, false); // Causes gpiopin to appear in /sys/class/gpio
  
  irqNumberPPS = gpio_to_irq(PPS_SIGNAL_PIN); // GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us
  if (irqNumberPPS < 0) return irqNumberPPS;
  // This next call requests an interrupt line
  result = request_irq(irqNumberPPS,  // The interrupt number requested
              (irq_handler_t) timestamp_irq_handler,  // The pointer to the handler function below
              INTERRUPT_TRIGGER,  // Interrupt on rising edge (button press, not release)
              "PPS interrupt",  // Used in /proc/interrupts to identify the owner
              NULL);  // The *dev_id for shared interrupt lines, NULL is okay

  //    FLASH     //
  result = gpio_request(FLASH_SIGNAL_PIN, "FLASH"); // Set up the gpio
  if (result < 0) return result;

  result = gpio_direction_input(FLASH_SIGNAL_PIN);  // Set the button GPIO to be an input
  if (result < 0) return result;
  gpio_set_debounce(FLASH_SIGNAL_PIN, 200); // Debounce the button with a delay of 200ms
  gpio_export(FLASH_SIGNAL_PIN, false); // Causes gpiopin to appear in /sys/class/gpio
  
  irqNumberFLASH = gpio_to_irq(FLASH_SIGNAL_PIN); // GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us
  if (irqNumberFLASH < 0) return irqNumberFLASH;
  // This next call requests an interrupt line
  result = request_irq(irqNumberFLASH,  // The interrupt number requested
              (irq_handler_t) timestamp_irq_handler,  // The pointer to the handler function below
              INTERRUPT_TRIGGER,  // Interrupt on rising edge (button press, not release)
              "FLASH interrupt",  // Used in /proc/interrupts to identify the owner
              NULL);  // The *dev_id for shared interrupt lines, NULL is okay
  return 0;
} 
// Gpio close
static void timestamp_gpio_close(void) {
  gpio_unexport(PPS_SIGNAL_PIN);                              // Unexport the GPIO
  gpio_unexport(FLASH_SIGNAL_PIN);                            // Unexport the GPIO
  free_irq(irqNumberPPS, NULL);                               // Free the IRQ number, no *dev_id required in this case
  free_irq(irqNumberFLASH, NULL);                             // Free the IRQ number, no *dev_id required in this case
  gpio_free(PPS_SIGNAL_PIN);                                  // Free the GPIO
  gpio_free(FLASH_SIGNAL_PIN);                                // Free the GPIO
}

// Custom file operations
static ssize_t tstamp_read(struct file *filp, char __user *buff, size_t count, loff_t *offp) {
  ssize_t bytesUnread;
  printk("Trying to read %6u bytes from driver.", count);
  bytesUnread = copy_to_user(buff, tstampArrayPtr, count);
  printk("Read %6u bytes from driver.", count - bytesUnread);
  return bytesUnread;
}
static int tstamp_open(struct inode *inode, struct file *filp) {
  printk("Timestamp driver open!");
  return 0;
}


/// This next calls are  mandatory -- they identify the initialization function
/// and the cleanup function (as above).
module_init(timestamp_init);
module_exit(timestamp_exit);
