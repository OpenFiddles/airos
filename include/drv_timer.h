#ifndef AIROS_DRV_TIMER_H
#define AIROS_DRV_TIMER_H

#include "drv.h"

#define DRV_TIMER_HZ 100u
#define DRV_PIT_INPUT_HZ 1193182u

extern struct drv drv_timer_device;

u32 drv_ticks(void);
/* Halts between ticks, so interrupts must be enabled or this never returns. */
void drv_sleep_ticks(u32 n);

#endif
