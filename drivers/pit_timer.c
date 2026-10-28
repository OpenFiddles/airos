/**
 * ../drivers/pit_timer.c - Extracted Programmable Interval Timer Driver
 */

#include "../kernel/kernel.h"

static int pit_driver_init(void);

/* Instantiate the generic interface driver block */
static struct driver pit_driver = {
    .name = "Intel 8253 PIT",
    .type = DRV_TYPE_TIMER,
    .init = pit_driver_init,
    .shutdown = 0
};

static int pit_driver_init(void) {
    u16 divisor = 11932; /* ~100Hz frequency mapping */
    outb(0x43, 0x36);
    outb(0x40, (u8)(divisor & 0xFF));
    outb(0x40, (u8)(divisor >> 8));
    return 1; // Return status success
}

// Change your raw IRQ to a standard C callback that your main IDT caller triggers
void timer_handler_callback(void) {
    ticks++;
    outb(0x20, 0x20); // Send End of Interrupt (EOI) to Master PIC
}

/* Automatically hook into registry layout during startup compilation */
void pit_driver_bootstrap(void) {
    driver_register(&pit_driver);
}
