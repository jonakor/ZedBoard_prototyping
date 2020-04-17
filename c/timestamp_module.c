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

MODULE_LICENSE("GPL");


/* FOR ZEDBOARD */
#define PPS_MIO_PIN 13 // JE1 physical pin on ZedBoard
#define FLASH_MIO_PIN 12 // JE4 physical pin on ZedBoard

/* FOR PICOZED */
//#define PPS_MIO_PIN 9
//#define FLASH_MIO_PIN 47

#define GPIO_MIO_OFFSET 906
#define PPS_GPIO_PIN (GPIO_MIO_OFFSET + PPS_MIO_PIN)    //MIO pin 50 on ZedBoard. Mapped to 906 + mio_pin = 956
#define FLASH_GPIO_PIN (GPIO_MIO_OFFSET + FLASH_MIO_PIN)  //MIO pin 51 on ZedBoard. Mapped to 906 + mio_pin = 957
static int timestamp_gpio_setup(void); // GPIO and interrupt setup function
static void timestamp_gpio_close(void); // GPIO and interrupt close function

// Timestamping
#define N_FRAME_MAX 2300
#define TIMESTAMP_ARRAY_SIZE (N_FRAME_MAX * 2 * 4) // Two timestamps per frame, four bytes per timestamp.
#define microsSinceUTC( nPPS, ticks) ((nPPS*1000000) + (ticks/AXI_TICKS_PER_MICROS)) // Custom macro for combining PPScount and timer ticks
static unsigned int *tstampArrayPtr;

// AXI Timer
static unsigned int *timerPtr; // Pointer to timer address

// Custom device driver
static struct cdev custom_dev;
static char* device_name = "timestamp";
static dev_t device_number;
static int tstamp_open(struct inode *inode, struct file *filp);
static ssize_t tstamp_read(struct file *filp, char __user *buff, size_t count, loff_t *offp);
static long tstamp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static struct class *cl;
static void *dev_id;

//Interrupt
#define INTERRUPT_TRIGGER (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING) //Make interrupts trigger on both edges
static irq_handler_t  timestamp_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs); // Interrupt handler.
static unsigned int irqNumberPPS; // Mapped IRQ number
static unsigned int irqNumberFLASH; // Mapped IRQ number
static unsigned int ppsCount;
static unsigned int frameCount;


// Initialization of module
static int __init timestamp_init(void){
  int result = 0;

  //Initialize structure for file operations
  static struct file_operations tstamp_fops = {
    .owner= THIS_MODULE,
    .open = tstamp_open,
    .read = tstamp_read,
    .unlocked_ioctl = tstamp_ioctl
  };

  //Initialize Driver By Getting Device Number
  if (alloc_chrdev_region(&device_number, 0, 1, device_name) != 0) {
    printk(KERN_ERR "Can't Allocate Device Number\n");
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
    printk(KERN_ERR "Couldn't map timer physical address: %x\n", TIMER_BASE_ADDRESS);
    return -1;
  }
  hyp_timer_setup(timerPtr);

  // Allocate timestamp array
  tstampArrayPtr = kzalloc(TIMESTAMP_ARRAY_SIZE, GFP_KERNEL);  

  printk(KERN_INFO "Timestamp module loaded.\n");
  return 0;
}

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


  printk(KERN_INFO "Timestamp module unloaded.\n");
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

  static bool prevPpsSignal = 0;
  static bool prevFlashSignal = 0;

  bool ppsSignal  = gpio_get_value(PPS_GPIO_PIN);
  bool flashSignal = gpio_get_value(FLASH_GPIO_PIN);

  //PPS signal
  if (ppsSignal != prevPpsSignal) {
     if (ppsSignal == 1){
        hyp_timer_reset(timerPtr);
        if (frameCount > 0) ppsCount ++;
        //printk(KERN_DEBUG "\nInterrupt!: Rising edge of PPS. PPS-count: %u\n", ppsCount);
     }
     else if (ppsSignal == 0) {
        //printk(KERN_DEBUG "\nInterrupt!: Falling edge of PPS.\n");
     }
  }

  //Get ticks since the last PPS
  ticks = hyp_timer_getTime(timerPtr);

  //Flash signal
  if (flashSignal != prevFlashSignal) {
    if (flashSignal == 1) {
      tstampArrayPtr[frameCount*2] = microsSinceUTC(ppsCount, ticks);
      //printk(KERN_DEBUG "\nInterrupt!: Rising edge of FLASH.\n");
      //printk(KERN_DEBUG "\nFrame %4u starts\tUTC-startstamp + %9u us (microseconds)\n", frameCount, microsSinceUTC(ppsCount, ticks));      
    }
    else if (flashSignal == 0) {
      tstampArrayPtr[frameCount*2 + 1] = microsSinceUTC(ppsCount, ticks);
      //printk(KERN_DEBUG "\nInterrupt!: Falling edge of FLASH.\n");
      //printk(KERN_DEBUG "Frame %4u ends  \tUTC-startstamp + %9u us (microseconds)\n", frameCount, microsSinceUTC(ppsCount, ticks));
      frameCount++;
    }
  }

  prevPpsSignal = ppsSignal;
  prevFlashSignal = flashSignal;

  return (irq_handler_t) IRQ_HANDLED;      // Announce that the IRQ has been handled correctly
}

// Gpio setup.
static int timestamp_gpio_setup(void) {
  int result;
  //    PPS     //
  result = gpio_request(PPS_GPIO_PIN, "PPS"); // Set up the gpio
  if (result < 0) return result;

  result = gpio_direction_input(PPS_GPIO_PIN);  // Set the button GPIO to be an input
  if (result < 0) return result;
  //gpio_set_debounce(PPS_GPIO_PIN, 200); // Debounce the button with a delay of 200ms
  //gpio_export(PPS_GPIO_PIN, false); // Causes gpiopin to appear in /sys/class/gpio
  
  irqNumberPPS = gpio_to_irq(PPS_GPIO_PIN); // GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us
  if (irqNumberPPS < 0) return irqNumberPPS;

  //    FLASH     //
  result = gpio_request(FLASH_GPIO_PIN, "FLASH"); // Set up the gpio
  if (result < 0) return result;

  result = gpio_direction_input(FLASH_GPIO_PIN);  // Set the button GPIO to be an input
  if (result < 0) return result;
  //gpio_set_debounce(FLASH_GPIO_PIN, 200); // Debounce the button with a delay of 200ms
  //gpio_export(FLASH_GPIO_PIN, false); // Causes gpiopin to appear in /sys/class/gpio
  
  irqNumberFLASH = gpio_to_irq(FLASH_GPIO_PIN); // GPIO numbers and IRQ numbers are not the same! This function performs the mapping for us
  if (irqNumberFLASH < 0) return irqNumberFLASH;

  return 0;
} 
// Gpio close
static void timestamp_gpio_close(void) {
  //gpio_unexport(PPS_GPIO_PIN);    // Unexport the GPIO
  //gpio_unexport(FLASH_GPIO_PIN);  // Unexport the GPIO
  free_irq(irqNumberPPS, dev_id);   // Free the IRQ number
  free_irq(irqNumberFLASH, dev_id); // Free the IRQ number
  gpio_free(PPS_GPIO_PIN);      // Free the GPIO
  gpio_free(FLASH_GPIO_PIN);    // Free the GPIO
}

// Custom file operations
static ssize_t tstamp_read(struct file *filp, char __user *buff, size_t count, loff_t *offp) {
  //printk(KERN_DEBUG "Trying to read %6u bytes from driver...\n", count);
  return copy_to_user(buff, tstampArrayPtr, count);
}
static int tstamp_open(struct inode *inode, struct file *filp) {
  //printk(KERN_DEBUG "Timestamp driver opened\n");
  return 0;
}
static long tstamp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
  long result;
  switch (cmd)
  {
    case 0:
      //printk(KERN_INFO "Enabling interrupts\n");
      ppsCount = 0;
      frameCount = 0;
      hyp_timer_start(timerPtr);

      result = request_irq(irqNumberPPS,  // The interrupt number requested
              (irq_handler_t) timestamp_irq_handler,  // The pointer to the handler function
              INTERRUPT_TRIGGER,  // Interrupt on rising edge (button press, not release)
              "PPS interrupt",  // Used in /proc/interrupts to identify the owner
              dev_id);  // The *dev_id for shared interrupt lines, NULL is okay
      if (result < 0) return result;

      result = request_irq(irqNumberFLASH,  // The interrupt number requested
              (irq_handler_t) timestamp_irq_handler,  // The pointer to the handler function
              INTERRUPT_TRIGGER,  // Interrupt on rising edge (button press, not release)
              "FLASH interrupt",  // Used in /proc/interrupts to identify the owner
              dev_id);  // The *dev_id for shared interrupt lines, NULL is okay
      if (result < 0) return result;
      break;

    case 1:
      //printk(KERN_INFO "Disabling interrupts\n");
      hyp_timer_stop(timerPtr);

      free_irq(irqNumberPPS, dev_id);   // Free the IRQ number
      free_irq(irqNumberFLASH, dev_id); // Free the IRQ number
      break;

    default:
      printk(KERN_INFO "No such command\n");
  }
  return 0;
}


/// This next calls are  mandatory -- they identify the initialization function
/// and the cleanup function (as above).
module_init(timestamp_init);
module_exit(timestamp_exit);
