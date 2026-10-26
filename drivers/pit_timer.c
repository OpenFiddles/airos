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

void timer_irq(void) {
    ticks++;
    outb(0x20, 0x20); // Send EOI
}

void wait_ticks(u32 n) {
    u32 start = ticks;
    while (ticks - start < n) {
        halt();
    }
}

/* Automatically hook into registry layout during startup compilation */
void pit_driver_bootstrap(void) {
    driver_register(&pit_driver);
}
