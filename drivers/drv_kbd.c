#include "../include/drv_kbd.h"
#include "../include/drv_pic.h"
#include "../include/drv_ring.h"
#include "../include/hw.h"

#define KBD_DATA 0x60
#define KBD_STATUS 0x64
#define KBD_STATUS_FULL 0x01
#define KBD_IRQ 1

static const char kbd_plain[0x3A] = {
    [0x01] = 27, [0x02] = '1', [0x03] = '2', [0x04] = '3', [0x05] = '4', [0x06] = '5',
    [0x07] = '6', [0x08] = '7', [0x09] = '8', [0x0A] = '9', [0x0B] = '0', [0x0C] = '-',
    [0x0D] = '=', [0x0E] = '\b', [0x0F] = '\t',
    [0x10] = 'q', [0x11] = 'w', [0x12] = 'e', [0x13] = 'r', [0x14] = 't', [0x15] = 'y',
    [0x16] = 'u', [0x17] = 'i', [0x18] = 'o', [0x19] = 'p', [0x1A] = '[', [0x1B] = ']',
    [0x1C] = '\n',
    [0x1E] = 'a', [0x1F] = 's', [0x20] = 'd', [0x21] = 'f', [0x22] = 'g', [0x23] = 'h',
    [0x24] = 'j', [0x25] = 'k', [0x26] = 'l', [0x27] = ';', [0x28] = '\'', [0x29] = '`',
    [0x2B] = '\\',
    [0x2C] = 'z', [0x2D] = 'x', [0x2E] = 'c', [0x2F] = 'v', [0x30] = 'b', [0x31] = 'n',
    [0x32] = 'm', [0x33] = ',', [0x34] = '.', [0x35] = '/', [0x37] = '*', [0x39] = ' ',
};

static const char kbd_shift_sym[0x3A] = {
    [0x02] = '!', [0x03] = '@', [0x04] = '#', [0x05] = '$', [0x06] = '%', [0x07] = '^',
    [0x08] = '&', [0x09] = '*', [0x0A] = '(', [0x0B] = ')', [0x0C] = '_', [0x0D] = '+',
    [0x1A] = '{', [0x1B] = '}', [0x27] = ':', [0x28] = '"', [0x29] = '~', [0x2B] = '|',
    [0x33] = '<', [0x34] = '>', [0x35] = '?',
};

static struct drv_ring kbd_ring;
static int shift_down;
static int extended;

static int kbd_translate(u8 sc) {
    char c;
    if (sc >= sizeof(kbd_plain))
        return -1;
    c = kbd_plain[sc];
    if (!c)
        return -1;
    if (shift_down) {
        if (c >= 'a' && c <= 'z')
            return c - 32;
        if (kbd_shift_sym[sc])
            return kbd_shift_sym[sc];
    }
    return (unsigned char)c;
}

static int kbd_extended(u8 sc) {
    switch (sc) {
    case 0x48: return DRV_KEY_UP;
    case 0x50: return DRV_KEY_DOWN;
    case 0x4B: return DRV_KEY_LEFT;
    case 0x4D: return DRV_KEY_RIGHT;
    case 0x1C: return DRV_KEY_ENTER;
    default: return -1;
    }
}

static void kbd_irq(void) {
    u8 sc = hw_inb(KBD_DATA);
    int was_extended;
    int key = -1;
    if (sc == 0xE0) {
        extended = 1;
        return;
    }
    was_extended = extended;
    extended = 0;
    if (sc & 0x80) {
        u8 code = sc & 0x7F;
        if (!was_extended && (code == 0x2A || code == 0x36))
            shift_down = 0;
        return;
    }
    if (was_extended) {
        key = kbd_extended(sc);
    } else if (sc == 0x2A || sc == 0x36) {
        shift_down = 1;
        return;
    } else {
        key = kbd_translate(sc);
    }
    if (key >= 0)
        drv_ring_put(&kbd_ring, (u16)key);
}

static int kbd_init(void) {
    drv_ring_reset(&kbd_ring);
    shift_down = 0;
    extended = 0;
    while (hw_inb(KBD_STATUS) & KBD_STATUS_FULL)
        (void)hw_inb(KBD_DATA);
    drv_pic_unmask(KBD_IRQ);
    return 0;
}

static const struct drv_ops drv_kbd_ops = { kbd_init, kbd_irq, 0 };

struct drv drv_kbd_device = { "keyboard", DRV_CLASS_INPUT, &drv_kbd_ops, KBD_IRQ, 0 };

int drv_kbd_getc(void) {
    return drv_ring_get(&kbd_ring);
}
