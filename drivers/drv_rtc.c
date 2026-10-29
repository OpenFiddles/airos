#include "../include/drv_rtc.h"
#include "../include/hw.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71
#define CMOS_STATUS_A 0x0A
#define CMOS_STATUS_B 0x0B

static u8 cmos(u8 reg) {
    hw_outb(CMOS_ADDR, reg);
    hw_io_wait();
    return hw_inb(CMOS_DATA);
}

static u8 bcd(u8 v) {
    return (u8)((v & 0x0F) + (v >> 4) * 10);
}

static int rtc_init(void) {
    return 0;
}

static const struct drv_ops drv_rtc_ops = { rtc_init, 0, 0 };

struct drv drv_rtc_device = { "rtc", DRV_CLASS_RTC, &drv_rtc_ops, DRV_NO_IRQ, 0 };

int drv_rtc_read(struct drv_time *t) {
    u8 status_b;
    if (!t)
        return -1;
    while (cmos(CMOS_STATUS_A) & 0x80)
        ;
    t->sec = cmos(0x00);
    t->min = cmos(0x02);
    t->hour = cmos(0x04) & 0x7F;
    t->day = cmos(0x07);
    t->month = cmos(0x08);
    t->year = cmos(0x09);
    status_b = cmos(CMOS_STATUS_B);
    if (!(status_b & 0x04)) {
        t->sec = bcd(t->sec);
        t->min = bcd(t->min);
        t->hour = bcd(t->hour);
        t->day = bcd(t->day);
        t->month = bcd(t->month);
        t->year = bcd(t->year);
    }
    t->year = (u16)(2000 + t->year);
    return 0;
}
