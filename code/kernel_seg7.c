#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include "address_map_arm.h"
#include "interrupt_ID.h"

/* This is a kernel module that uses interrupts from the KEY port. The interrupt service 
 * routine increments a value. The LSB is displayed as a BCD digit on the display HEX0, 
 * and the value is also displayed as a binary number on the red lights LEDR. */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Altera University Program");
MODULE_DESCRIPTION("DE1-SoC Computer Pushbutton Interrupt Handler");

// 7-seg bit patterns for digits 0-9
char seg7[10] =   {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 
                   0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01100111};

void *LW_virtual;             // used to map physical addresses for the light-weight bridge
volatile int *LEDR_ptr;       // virtual address for the LEDR port
volatile int *KEY_ptr;        // virtual address for the KEY port
volatile int * HEX3_HEX0_ptr; // virtual pointer to HEX displays

irq_handler_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
   int value;
   // Increment the value on the LEDs
   value = *LEDR_ptr;
   ++value;
   *LEDR_ptr = value;

	printk(KERN_INFO "Interrupt called\n");

   if ((value & 0xF) > 9)    // ignore upper bit when testing value
   {
      value = 0x200;         // leave upper bit turned on
      *LEDR_ptr = value;
   }
   // display least-sig BCD digit on 7-segment display
   *HEX3_HEX0_ptr = seg7[value & 0xF];

   // Clear the edgecapture register (clears current interrupt)
   *(KEY_ptr + 3) = 0xF; 
   
   return (irq_handler_t) IRQ_HANDLED;
}

static int __init intitialize_pushbutton_handler(void)
{
   // generate a virtual address for the FPGA lightweight bridge
   LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN);

   LEDR_ptr = LW_virtual + LEDR_BASE;   // init virtual address for LEDR port
   *LEDR_ptr = 0x200;                   // turn on the leftmost light

   KEY_ptr = LW_virtual + KEY_BASE;     // init virtual address for KEY port
   // Clear the PIO edgecapture register (clear any pending interrupt)
   *(KEY_ptr + 3) = 0xF; 
   // Enable IRQ generation for the 4 buttons
   *(KEY_ptr + 2) = 0xF; 

   HEX3_HEX0_ptr = LW_virtual + HEX3_HEX0_BASE;   // init virtual address for HEX port
   *HEX3_HEX0_ptr = seg7[0];   // display 0
   // register the interrupt handler, and then return
   return request_irq (KEY_IRQ, (irq_handler_t) irq_handler, IRQF_SHARED,
      "pushbutton_irq_handler", (void *) (irq_handler));
}

static void __exit cleanup_pushbutton_handler(void)
{
   *LEDR_ptr = 0; // Turn off LEDs and de-register irq handler
   iounmap (LW_virtual);
   free_irq (KEY_IRQ, (void*) irq_handler);
}

module_init (intitialize_pushbutton_handler);
module_exit (cleanup_pushbutton_handler);
