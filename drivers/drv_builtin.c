#include "../include/drv.h"
#include "../include/drv_pic.h"
#include "../include/drv_timer.h"
#include "../include/drv_serial.h"
#include "../include/drv_kbd.h"
#include "../include/drv_rtc.h"
#include "../include/drv_speaker.h"

/* Order matters: the interrupt controller must come before anything that unmasks an IRQ. */
int drv_register_builtin(void) {
    struct drv *list[] = {
        &drv_pic_device,
        &drv_timer_device,
        &drv_serial_device,
        &drv_kbd_device,
        &drv_rtc_device,
        &drv_speaker_device,
    };
    int ok = 0;
    for (unsigned int i = 0; i < sizeof(list) / sizeof(list[0]); i++)
        if (drv_register(list[i]) == 0)
            ok++;
    return ok;
}
