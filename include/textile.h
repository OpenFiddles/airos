#ifndef TEXTILE_H
#define TEXTILE_H

/* App ABI: a flat binary linked at TX_APP_BASE whose first two words are
   TX_APP_MAGIC and the address of app_main. Syscalls use int 0x80:
   eax = number, ebx/ecx/edx = arguments, return value in eax. */
#define TX_APP_MAGIC 0x54505041u
#define TX_APP_BASE 0x20000u

enum tx_syscall {
    SYS_PUTC = 1,
    SYS_PUTAT = 2,
    SYS_CLEAR = 3,
    SYS_TICKS = 4,
    SYS_SLEEP = 5,
    SYS_TONE = 6,
    SYS_ANIM_START = 7,
    SYS_ANIM_VALUE = 8,
    SYS_GETKEY = 9,
    SYS_PRINT = 10,
    SYS_EXIT = 11
};

enum tx_ease { EASE_LINEAR = 0, EASE_OUT_CUBIC = 1, EASE_IN_OUT = 2 };

#endif
