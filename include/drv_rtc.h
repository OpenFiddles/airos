#ifndef AIROS_DRV_RTC_H
#define AIROS_DRV_RTC_H

#include "drv.h"

struct drv_time {
    u8 sec;
    u8 min;
    u8 hour;
    u8 day;
    u8 month;
    u16 year;
};

extern struct drv drv_rtc_device;

int drv_rtc_read(struct drv_time *t);

#endif
