#ifndef AIROS_DRV_TOUCH_H
#define AIROS_DRV_TOUCH_H

#include "drv.h"

#define TOUCH_RING 16
#define TOUCH_LINE_MAX 32

struct touch_ev {
    unsigned short x;
    unsigned short y;
    unsigned char down;
};

int touch_poll(void);
int touch_next(struct touch_ev *out);

extern struct drv drv_touch_device;

#endif
