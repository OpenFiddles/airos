/* AirOS kernel: VGA console, COM1 mirror, PIC, PIT, PS/2 keyboard, CMOS clock,
   NE2000 NIC, tween engine, PC speaker, int 0x80 syscall gate and a flat app loaded at TX_APP_BASE. */

#include "textile.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef int i32;

#define VGA_COLS 80
#define VGA_ROWS 25
#define VGA_MEM ((volatile u16 *)0xB8000)
#define COM1 0x3F8
#define ATTR_TEXT 0x07
#define ATTR_OK 0x0A
#define ATTR_ERR 0x0C
#define ATTR_SQR 0x0B
#define PCI_ADDR 0xCF8
#define PCI_DATA 0xCFC
#define RTL_VENDOR 0x10EC
#define RTL_DEVICE 0x8139
#define ANIM_SLOTS 16
#define PCI_SLOT_MAX 32

/* Driver layer */
extern int drv_register_builtin(void);
extern int drv_init_all(void);
extern void drv_shutdown_all(void);

static inline void outb(u16 port, u8 value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void outl(u16 port, u32 value) {
    __asm__ volatile ("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline u32 inl(u16 port) {
    u32 value;
    __asm__ volatile ("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void io_wait(void) { outb(0x80, 0); }
static inline void irq_on(void) { __asm__ volatile ("sti"); }
static inline void irq_off(void) { __asm__ volatile ("cli"); }
static inline void halt(void) { __asm__ volatile ("hlt"); }

void *memset(void *dst, int value, u32 n) {
    u8 *p = dst;
    while (n--)
        *p++ = (u8)value;
    return dst;
}

void *memcpy(void *dst, const void *src, u32 n) {
    u8 *d = dst;
    const u8 *s = src;
    while (n--)
        *d++ = *s++;
    return dst;
}

static int cur_row, cur_col;
static u8 cur_attr = ATTR_TEXT;

#define TERM_COLS 56
#define TERM_KEEP 32
#define TERM_INPUT_MAX 50

int kbd_toggle_take(void);

static char term_buf[TERM_KEEP][TERM_COLS];
static int term_len[TERM_KEEP];
static int term_total = 1;
int term_capture;

static void term_put(char c) {
    int cur = (term_total - 1) % TERM_KEEP;
    if (c == '\n') {
        term_total++;
        term_len[(term_total - 1) % TERM_KEEP] = 0;
        return;
    }
    if (c == '\b') {
        if (term_len[cur] > 0)
            term_len[cur]--;
        return;
    }
    if (term_len[cur] >= TERM_COLS) {
        term_total++;
        cur = (term_total - 1) % TERM_KEEP;
        term_len[cur] = 0;
    }
    term_buf[cur][term_len[cur]++] = c;
}

static void term_clear(void) {
    term_total = 1;
    term_len[0] = 0;
}

int term_count(void) {
    return term_total;
}

const char *term_text(int idx, int *len) {
    *len = term_len[idx % TERM_KEEP];
    return term_buf[idx % TERM_KEEP];
}


static void serial_init(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

static void serial_putc(char c) {
    while (!(inb(COM1 + 5) & 0x20))
        ;
    outb(COM1, (u8)c);
}

static void update_cursor(void) {