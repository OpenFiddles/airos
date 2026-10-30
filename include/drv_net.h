#ifndef AIROS_DRV_NET_H
#define AIROS_DRV_NET_H

#include "drv.h"

/* Network driver interface. The driver auto-detects the hardware:
   NE2000 at I/O 0x300, IRQ 10 (v86 browser) or
   RTL8139 at I/O 0xC000, IRQ 11 (QEMU). */

extern struct drv drv_net_device;

const u8 *drv_net_mac(void);
u16 drv_net_io(void);
int drv_net_is_rtl(void);
int drv_net_init(void);

#endif
