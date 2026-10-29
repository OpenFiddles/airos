#ifndef AIROS_DRV_KBD_H
#define AIROS_DRV_KBD_H

#include "drv.h"

#define DRV_KEY_BACKSPACE 8
#define DRV_KEY_TAB 9
#define DRV_KEY_ENTER 10
#define DRV_KEY_ESC 27
#define DRV_KEY_UP 256
#define DRV_KEY_DOWN 257
#define DRV_KEY_LEFT 258
#define DRV_KEY_RIGHT 259

extern struct drv drv_kbd_device;

/* Returns the next key (ASCII, or one of the DRV_KEY_ codes above), or -1 when none is waiting. */
int drv_kbd_getc(void);

#endif
