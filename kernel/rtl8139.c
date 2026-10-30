#include "../include/drv.h"
#include "../include/drv_pic.h"
#include "../include/hw.h"

/*
 * NE2000-compatible NIC driver for v86.
 *
 * v86 defaults to an NE2000 (RTL8390) PCI card at I/O port 0x300, IRQ 10.
 * This driver uses the same register layout as the original RTL8139 code
 * but maps it to the NE2000 I/O window so it works in the browser build.
 */

#define NE2K_IO      0x300
#define NE2K_IRQ     10
#define NE2K_RESET   0x1F
#define NE2K_CMD     0x00
#define NE2K_ISR     0x07
#define NE2K_IMR     0x0F
#define NE2K_RCR     0x0C
#define NE2K_TCR     0x0D
#define NE2K_DCFG    0x0E
#define NE2K_BNRY    0x03
#define NE2K_STARTPG 0x01
#define NE2K_STOPPG  0x02
#define NE2K_RSARLO  0x08
#define NE2K_RSARHI  0x09
#define NE2K_RCNTLO  0x0A
#define NE2K_RCNTHI  0x0B
#define NE2K_TPSR    0x04
#define NE2K_TCNTLO  0x05
#define NE2K_TCNTHI  0x06
#define NE2K_PSTART  0x40
#define NE2K_PSTOP   0x80

static u16 ne2k_io;
static u8 ne2k_mac[6];

static int ne2k_init(void) {
    ne2k_io = NE2K_IO;
    /* Software reset */
    outb(ne2k_io + NE2K_RESET, 0x00);
    /* Set receive buffer: start at page 0x40, stop at 0x80 */
    outb(ne2k_io + NE2K_STARTPG, NE2K_PSTART);
    outb(ne2k_io + NE2K_STOPPG, NE2K_PSTOP);
    outb(ne2k_io + NE2K_BNRY, NE2K_PSTART);
    /* Accept broadcast + physical match + promiscuous */
    outb(ne2k_io + NE2K_RCR, 0x8F);
    /* Disable interrupts initially */
    outb(ne2k_io + NE2K_IMR, 0x00);
    /* Enable receiver and transmitter */
    outb(ne2k_io + NE2K_CMD, 0x0C);
    /* Read factory MAC from ROM */
    outb(ne2k_io + NE2K_RSARLO, 0x00);
    outb(ne2k_io + NE2K_RSARHI, 0x00);
    outb(ne2k_io + NE2K_RCNTLO, 6);
    outb(ne2k_io + NE2K_RCNTHI, 0);
    for (int i = 0; i < 6; i++)
        ne2k_mac[i] = inb(ne2k_io + NE2K_DCFG);
    /* Unmask IRQ */
    outb(ne2k_io + NE2K_IMR, 0xFF);
    drv_pic_unmask(NE2K_IRQ);
    return 0;
}

static void ne2k_irq(void) {
    u8 isr = inb(ne2k_io + NE2K_ISR);
    if (isr & 0x01) {
        /* Receiver interrupt */
    }
    if (isr & 0x02) {
        /* Transmitter interrupt */
    }
    if (isr & 0x80) {
        /* Reset complete */
    }
}

static void ne2k_shutdown(void) {
    outb(ne2k_io + NE2K_IMR, 0x00);
    outb(ne2k_io + NE2K_CMD, 0x00);
    drv_pic_mask(NE2K_IRQ);
}

static const struct drv_ops drv_net_ops = { ne2k_init, ne2k_irq, ne2k_shutdown };

struct drv drv_net_device = { "ne2k", DRV_CLASS_INPUT, &drv_net_ops, NE2K_IRQ, 0 };

static int net_link_up(void) {
    return !(inb(ne2k_io + 0x05) & 0x04);
}

void drv_net_put_mac(const u8 *mac) {
    for (int i = 0; i < 6; i++) {
        put_hexn(mac[i], 2);
        if (i < 5)
            putc(':');
    }
}

const u8 *drv_net_mac(void) { return ne2k_mac; }
u16 drv_net_io(void) { return ne2k_io; }
