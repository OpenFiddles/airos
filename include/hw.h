#ifndef AIROS_HW_H
#define AIROS_HW_H

#include "airos_types.h"

static inline void hw_outb(u16 port, u8 value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline u8 hw_inb(u16 port) {
    u8 value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void hw_outw(u16 port, u16 value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline u16 hw_inw(u16 port) {
    u16 value;
    __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void hw_outl(u16 port, u32 value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline u32 hw_inl(u16 port) {
    u32 value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void hw_io_wait(void) { hw_outb(0x80, 0); }
static inline void hw_irq_enable(void) { __asm__ volatile ("sti"); }
static inline void hw_irq_disable(void) { __asm__ volatile ("cli"); }
static inline void hw_halt(void) { __asm__ volatile ("hlt"); }

#endif
