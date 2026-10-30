#include "../include/drv.h"
#include "../include/drv_pic.h"
#include "../include/hw.h"

/*
 * AirOS network driver.
 *
 * v86 (browser) uses an NE2000-compatible NIC at I/O port 0x300, IRQ 10.
 * QEMU defaults to an RTL8139 at I/O port 0xC000, IRQ 11.
 *
 * The driver auto-detects which card is present by probing the I/O ports.
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

#define RTL_IO       0xC000
#define RTL_IRQ      11
#define RTL_REG_MAC0  0x00
#define RTL_REG_MAC4  0x04
#define RTL_REG_RBSTART 0x30
#define RTL_REG_CR    0x37
#define RTL_REG_CAPR  0x38
#define RTL_REG_IMR   0x3C
#define RTL_REG_ISR   0x3E
#define RTL_REG_TCR   0x40
#define RTL_REG_RCR   0x44
#define RTL_REG_CONFIG1 0x52
#define RTL_REG_MSR   0x58

#define RX_BUF_SIZE 8192

static u16 net_io;
static u8 net_mac[6];
static int is_rtl;

static u8 rx_buffer[RX_BUF_SIZE + 16 + 1500] __attribute__((aligned(4)));

static int net_link_up(void) {
    if (net_io == 0)
        return 0;
    if (is_rtl) {
        u8 msr = inb(net_io + RTL_REG_MSR);
        return (msr & 0x04) ? 0 : 1;
    }
    return 1;
}

static void net_put_mac(const u8 *mac) {
    for (int i = 0; i < 6; i++) {
        put_hexn(mac[i], 2);
        if (i < 5)
            putc(':');
    }
}

static int ne2k_init(void) {
    net_io = NE2K_IO;
    is_rtl = 0;

    outb(net_io + NE2K_RESET, 0x00);
    outb(net_io + NE2K_STARTPG, NE2K_PSTART);
    outb(net_io + NE2K_STOPPG, NE2K_PSTOP);
    outb(net_io + NE2K_BNRY, NE2K_PSTART);
    outb(net_io + NE2K_RCR, 0x8F);
    outb(net_io + NE2K_IMR, 0x00);
    outb(net_io + NE2K_CMD, 0x0C);

    outb(net_io + NE2K_RSARLO, 0x00);
    outb(net_io + NE2K_RSARHI, 0x00);
    outb(net_io + NE2K_RCNTLO, 6);
    outb(net_io + NE2K_RCNTHI, 0);
    for (int i = 0; i < 6; i++)
        net_mac[i] = inb(net_io + NE2K_DCFG);

    outb(net_io + NE2K_IMR, 0xFF);
    drv_pic_unmask(NE2K_IRQ);
    return 0;
}

static int rtl_init(void) {
    net_io = RTL_IO;
    is_rtl = 1;

    outb(net_io + RTL_REG_CONFIG1, 0x00);
    outb(net_io + RTL_REG_CR, 0x10);
    while ((inb(net_io + RTL_REG_CR) & 0x10) != 0)
        io_wait();

    outl(net_io + RTL_REG_RBSTART, (u32)&rx_buffer[0]);
    outw(net_io + RTL_REG_IMR, 0x0005);
    outl(net_io + RTL_REG_RCR, 0x0000008F);
    outb(net_io + RTL_REG_CR, 0x0C);

    u32 mac_low = inl(net_io + RTL_REG_MAC0);
    u16 mac_high = inw(net_io + RTL_REG_MAC4);
    net_mac[0] = (u8)(mac_low & 0xFF);
    net_mac[1] = (u8)((mac_low >> 8) & 0xFF);
    net_mac[2] = (u8)((mac_low >> 16) & 0xFF);
    net_mac[3] = (u8)(mac_low >> 24);
    net_mac[4] = (u8)(mac_high & 0xFF);
    net_mac[5] = (u8)((mac_high >> 8) & 0xFF);

    drv_pic_unmask(RTL_IRQ);
    return 0;
}

static void ne2k_irq(void) {
    u8 isr = inb(net_io + NE2K_ISR);
    (void)isr;
}

static void rtl_irq(void) {
    u16 isr = inw(net_io + RTL_REG_ISR);
    (void)isr;
}

static void net_shutdown(void) {
    if (net_io == 0)
        return;
    if (is_rtl) {
        outb(net_io + RTL_REG_CR, 0x00);
        outw(net_io + RTL_REG_IMR, 0x0000);
    } else {
        outb(net_io + NE2K_IMR, 0x00);
        outb(net_io + NE2K_CMD, 0x00);
    }
    drv_pic_mask(is_rtl ? RTL_IRQ : NE2K_IRQ);
}

static const struct drv_ops drv_net_ops = { 0, 0, net_shutdown };

struct drv drv_net_device = { "net", DRV_CLASS_CHAR, &drv_net_ops, DRV_NO_IRQ, 0 };

int drv_net_init(void) {
    /* Probe NE2000 first (v86 browser), then RTL8139 (QEMU). */
    u8 cfg1 = inb(NE2K_IO + NE2K_DCFG);
    if (cfg1 != 0xFF && cfg1 != 0x00) {
        ne2k_init();
        drv_net_device.irq_line = NE2K_IRQ;
    } else {
        /* Check for RTL8139 at 0xC000 */
        u32 id = inl(RTL_IO + RTL_REG_MAC0);
        if ((id & 0xFFFF) == RTL_VENDOR && (id >> 16) == RTL_DEVICE) {
            rtl_init();
            drv_net_device.irq_line = RTL_IRQ;
        } else {
            return -1;
        }
    }
    drv_net_device.ready = 1;
    return 0;
}

const u8 *drv_net_mac(void) { return net_mac; }
u16 drv_net_io(void) { return net_io; }
int drv_net_is_rtl(void) { return is_rtl; }