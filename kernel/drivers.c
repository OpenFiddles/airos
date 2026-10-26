/**
 * drivers.c - Generic Driver Core Manager
 * Controls core CPU/PIC frameworks and provides registration vectors for modular drivers.
 */

#include "kernel.h"

/* Allocation of Global Kernel States */
volatile u32 ticks = 0;
int cur_row = 0;
int cur_col = 0;
u8 cur_attr = ATTR_TEXT;

/* --- Internal Driver Registry Memory --- */
static struct driver *driver_registry[MAX_DRIVERS];
static int registered_driver_count = 0;

/* --- Core Interrupt Architecture Layouts --- */
struct idt_entry {
    u16 offset_low;
    u16 selector;
    u8 zero;
    u8 flags;
    u16 offset_high;
} __attribute__((packed));

struct idt_ptr {
    u16 limit;
    u32 base;
} __attribute__((packed));

static struct idt_entry idt[256];

static void idt_set(int n, void (*handler)(void), u8 flags) {
    u32 addr = (u32)handler;
    idt[n].offset_low = (u16)(addr & 0xFFFF);
    idt[n].selector = 0x08;
    idt[n].zero = 0;
    idt[n].flags = flags;
    idt[n].offset_high = (u16)(addr >> 16);
}

void idt_init(void) {
    for (int i = 0; i < 32; i++) idt_set(i, isr_fault, 0x8E);
    idt_set(0x20, isr_timer, 0x8E);
    idt_set(0x21, isr_keyboard, 0x8E);
    idt_set(0x80, isr_syscall, 0x8F);
    struct idt_ptr ptr = { (u16)(sizeof(idt) - 1), (u32)idt };
    __asm__ volatile ("lidt %0" : : "m"(ptr));
}

void pic_init(void) {
    outb(0x20, 0x11); io_wait(); outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait(); outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait(); outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait(); outb(0xA1, 0x01); io_wait();
    outb(0x21, 0xFC); outb(0xA1, 0xFF); // Unmask Master Timer and Keyboard
}

/* --- Core Generic Driver Operations --- */

/**
 * Registers an abstraction structure into the system table.
 * Allows modules compiled in ../drivers to hook their initialization sequences.
 */
int driver_register(struct driver *drv) {
    if (!drv || registered_driver_count >= MAX_DRIVERS) {
        return 0;
    }
    driver_registry[registered_driver_count++] = drv;
    return 1;
}

/**
 * Iterates through the registered subsystem array and boots up each device driver.
 */
int drivers_init_all(void) {
    int loaded = 0;
    for (int i = 0; i < registered_driver_count; i++) {
        if (driver_registry[i]->init) {
            if (driver_registry[i]->init()) {
                loaded++;
            }
        }
    }
    return loaded;
}
