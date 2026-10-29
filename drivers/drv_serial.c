#include "../include/drv_serial.h"
#include "../include/hw.h"

#define COM1 0x3F8
#define REG_DATA 0
#define REG_IER 1
#define REG_FCR 2
#define REG_LCR 3
#define REG_MCR 4
#define REG_LSR 5
#define LSR_DATA_READY 0x01
#define LSR_THR_EMPTY 0x20

static int serial_init(void) {
    hw_outb(COM1 + REG_IER, 0x00);
    hw_outb(COM1 + REG_LCR, 0x80);
    hw_outb(COM1 + REG_DATA, 0x03);
    hw_outb(COM1 + REG_IER, 0x00);
    hw_outb(COM1 + REG_LCR, 0x03);
    hw_outb(COM1 + REG_FCR, 0xC7);
    hw_outb(COM1 + REG_MCR, 0x0B);
    return 0;
}

static const struct drv_ops drv_serial_ops = { serial_init, 0, 0 };

struct drv drv_serial_device = { "serial", DRV_CLASS_CHAR, &drv_serial_ops, DRV_NO_IRQ, 0 };

void drv_serial_putc(char c) {
    while (!(hw_inb(COM1 + REG_LSR) & LSR_THR_EMPTY))
        ;
    hw_outb(COM1 + REG_DATA, (u8)c);
}

void drv_serial_write(const char *s) {
    while (*s) {
        if (*s == '\n')
            drv_serial_putc('\r');
        drv_serial_putc(*s++);
    }
}

int drv_serial_getc(void) {
    if (!(hw_inb(COM1 + REG_LSR) & LSR_DATA_READY))
        return -1;
    return hw_inb(COM1 + REG_DATA);
}
