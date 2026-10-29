#include "../include/drv_speaker.h"
#include "../include/drv_timer.h"
#include "../include/hw.h"

#define PIT_CH2 0x42
#define PIT_CMD 0x43
#define SPEAKER_CTRL 0x61

void drv_speaker_off(void) {
    hw_outb(SPEAKER_CTRL, hw_inb(SPEAKER_CTRL) & 0xFC);
}

void drv_speaker_tone(u32 hz) {
    u32 divisor;
    if (hz < 20 || hz > 20000) {
        drv_speaker_off();
        return;
    }
    divisor = DRV_PIT_INPUT_HZ / hz;
    hw_outb(PIT_CMD, 0xB6);
    hw_outb(PIT_CH2, (u8)(divisor & 0xFF));
    hw_outb(PIT_CH2, (u8)(divisor >> 8));
    hw_outb(SPEAKER_CTRL, hw_inb(SPEAKER_CTRL) | 0x03);
}

int drv_speaker_beep(u32 hz, u32 ms) {
    if (hz < 20 || hz > 20000)
        return -1;
    drv_speaker_tone(hz);
    drv_sleep_ticks((ms + 9) / 10);
    drv_speaker_off();
    return 0;
}

static int speaker_init(void) {
    drv_speaker_off();
    return 0;
}

static const struct drv_ops drv_speaker_ops = { speaker_init, 0, 0 };

struct drv drv_speaker_device = { "speaker", DRV_CLASS_AUDIO, &drv_speaker_ops, DRV_NO_IRQ, 0 };
