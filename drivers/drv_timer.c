#include "../include/drv_timer.h"
#include "../include/drv_pic.h"
#include "../include/hw.h"

#define PIT_CH0 0x40
#define PIT_CMD 0x43

static volatile u32 tick_count;

static int timer_init(void) {
    u32 divisor = DRV_PIT_INPUT_HZ / DRV_TIMER_HZ;
    tick_count = 0;
    hw_outb(PIT_CMD, 0x36);
    hw_outb(PIT_CH0, (u8)(divisor & 0xFF));
    hw_outb(PIT_CH0, (u8)(divisor >> 8));
    drv_pic_unmask(0);
    return 0;
}

static void timer_irq(void) {
    tick_count++;
}

static const struct drv_ops drv_timer_ops = { timer_init, timer_irq, 0 };

struct drv drv_timer_device = { "timer", DRV_CLASS_TIMER, &drv_timer_ops, 0, 0 };

u32 drv_ticks(void) {
    return tick_count;
}

void drv_sleep_ticks(u32 n) {
    u32 start = tick_count;
    while (tick_count - start < n)
        hw_halt();
}
