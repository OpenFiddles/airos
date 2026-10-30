#include "kernel.h"

/* NE2000-compatible NIC driver for v86.
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
#define NE2K_STARTPG 0x40
#define NE2K_PSTOP   0x80

static u16 ne2k_io;
static u8 ne2k_mac[6];

static int ne2k_init(void) {
    ne2k_io = NE2K_IO;
    /* Software reset */
    outb(ne2k_io + NE2K_RESET, 0x00);
    /* Set receive buffer: start at page 0x40, stop at 0x80 */
    outb(ne2k_io + NE2K_STARTPG, NE2K_STARTPG);
    outb(ne2k_io + NE2K_STOPPG, NE2K_PSTOP);
    outb(ne2k_io + NE2K_BNRY, NE2K_STARTPG);
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
    (void)isr;
}

static void ne2k_shutdown(void) {
    outb(ne2k_io + NE2K_IMR, 0x00);
    outb(ne2k_io + NE2K_CMD, 0x00);
    drv_pic_mask(NE2K_IRQ);
}

static const struct drv_ops drv_net_ops = { ne2k_init, ne2k_irq, ne2k_shutdown };

struct drv drv_net_device = { "ne2k", DRV_CLASS_INPUT, &drv_net_ops, NE2K_IRQ, 0 };

const u8 *drv_net_mac(void) { return ne2k_mac; }
u16 drv_net_io(void) { return ne2k_io; }
