#include "../include/drv_pic.h"
#include "../include/hw.h"

#define PIC1_CMD 0x20
#define PIC1_DATA 0x21
#define PIC2_CMD 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20
#define PIC1_BASE 0x20
#define PIC2_BASE 0x28
#define PIC_CASCADE_BIT 0x04

static int pic_init(void) {
    hw_outb(PIC1_CMD, 0x11);
    hw_io_wait();
    hw_outb(PIC2_CMD, 0x11);
    hw_io_wait();
    hw_outb(PIC1_DATA, PIC1_BASE);
    hw_io_wait();
    hw_outb(PIC2_DATA, PIC2_BASE);
    hw_io_wait();
    hw_outb(PIC1_DATA, 0x04);
    hw_io_wait();
    hw_outb(PIC2_DATA, 0x02);
    hw_io_wait();
    hw_outb(PIC1_DATA, 0x01);
    hw_io_wait();
    hw_outb(PIC2_DATA, 0x01);
    hw_io_wait();
    hw_outb(PIC1_DATA, 0xFF);
    hw_outb(PIC2_DATA, 0xFF);
    return 0;
}

static const struct drv_ops drv_pic_ops = { pic_init, 0, 0 };

struct drv drv_pic_device = { "pic", DRV_CLASS_INTC, &drv_pic_ops, DRV_NO_IRQ, 0 };

static void pic_set_mask(int line, int masked) {
    u16 port = line < 8 ? PIC1_DATA : PIC2_DATA;
    u8 bit = (u8)(1u << (line & 7));
    u8 m = hw_inb(port);
    m = masked ? (u8)(m | bit) : (u8)(m & (u8)~bit);
    hw_outb(port, m);
    if (line >= 8 && !masked) {
        u8 master = hw_inb(PIC1_DATA);
        hw_outb(PIC1_DATA, (u8)(master & (u8)~PIC_CASCADE_BIT));
    }
}

void drv_pic_mask(int line) {
    if (line >= 0 && line < 16)
        pic_set_mask(line, 1);
}

void drv_pic_unmask(int line) {
    if (line >= 0 && line < 16)
        pic_set_mask(line, 0);
}

void drv_pic_eoi(int line) {
    if (line < 0 || line >= 16)
        return;
    if (line >= 8)
        hw_outb(PIC2_CMD, PIC_EOI);
    hw_outb(PIC1_CMD, PIC_EOI);
}
