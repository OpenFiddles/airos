#include "../kernel/kernel.h"

#define MOUSE_SUB 4
#define MOUSE_MAX_X (VGA_COLS * MOUSE_SUB - 1)
#define MOUSE_MAX_Y (VGA_ROWS * MOUSE_SUB - 1)
#define PS2_TRIES 100000

volatile u8 mouse_buttons;
static u8 packet[3];
static u8 phase;
static i32 sub_x = 10 * MOUSE_SUB;
static i32 sub_y = 10 * MOUSE_SUB;

static int ctl_wait_write(void) {
    for (u32 i = 0; i < PS2_TRIES; i++)
        if (!(inb(0x64) & 0x02))
            return 1;
    return 0;
}

static int ctl_wait_read(void) {
    for (u32 i = 0; i < PS2_TRIES; i++)
        if (inb(0x64) & 0x01)
            return 1;
    return 0;
}

static int mouse_send(u8 cmd) {
    if (!ctl_wait_write())
        return 0;
    outb(0x64, 0xD4);
    if (!ctl_wait_write())
        return 0;
    outb(0x60, cmd);
    if (!ctl_wait_read())
        return 0;
    return inb(0x60) == 0xFA;
}

int mouse_init(void) {
    u8 cfg;
    if (!ctl_wait_write())
        return 0;
    outb(0x64, 0xA8);
    if (!ctl_wait_write())
        return 0;
    outb(0x64, 0x20);
    if (!ctl_wait_read())
        return 0;
    cfg = inb(0x60);
    cfg |= 0x02;
    cfg &= (u8)~0x20;
    if (!ctl_wait_write())
        return 0;
    outb(0x64, 0x60);
    if (!ctl_wait_write())
        return 0;
    outb(0x60, cfg);
    if (!mouse_send(0xF6))
        return 0;
    if (!mouse_send(0xF4))
        return 0;
    phase = 0;
    return 1;
}

void mouse_irq_handler(void) {
    u8 status = inb(0x64);
    u8 b;
    if (!(status & 0x20))
        return;
    b = inb(0x60);
    if (phase == 0 && !(b & 0x08))
        return;
    packet[phase++] = b;
    if (phase < 3)
        return;
    phase = 0;
    i32 dx = (i32)packet[1] - (i32)((packet[0] << 4) & 0x100);
    i32 dy = (i32)packet[2] - (i32)((packet[0] << 3) & 0x100);
    mouse_buttons = packet[0] & 0x07;
    sub_x += dx;
    sub_y -= dy;
    if (sub_x < 0)
        sub_x = 0;
    if (sub_x > MOUSE_MAX_X)
        sub_x = MOUSE_MAX_X;
    if (sub_y < 0)
        sub_y = 0;
    if (sub_y > MOUSE_MAX_Y)
        sub_y = MOUSE_MAX_Y;
    cursor_x = (u32)(sub_x / MOUSE_SUB);
    cursor_y = (u32)(sub_y / MOUSE_SUB);
}
