#ifndef AIROS_H
#define AIROS_H

/* AirOS app SDK: include this one header from an app in userland/ and link it with
   userland/app.ld. AIRP_APP(fn) marks fn as the app's main and emits the app head. */

#include "../kernel/textile.h"
#include "../include/axi.h"
#include "../include/fs.h"
#include "../include/airp.h"

#define AIRP_APP(main_fn) \
    static void airp_entry(void) { \
        main_fn(); \
        ax_call(SYS_EXIT, 0, 0, 0); \
        for (;;) \
            ; \
    } \
    __attribute__((section(".head"), used)) \
    const unsigned int airp_head[2] = { TX_APP_MAGIC, (unsigned int)(unsigned long)airp_entry };

#endif
