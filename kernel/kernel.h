#ifndef KERNEL_H
#define KERNEL_H

#include "textile.h"

/* Primary Typings */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef int i32;

/* Shared Consts */
#define VGA_COLS 80
#define VGA_ROWS 25
#define VGA_MEM ((volatile u16 *)0xB8000)
#define COM1 0x3F8

#define ATTR_TEXT 0x07
#define ATTR_OK 0x0A
#define ATTR_ERR 0x0C
#define ATTR_SQR 0x0B

#define ANIM_SLOTS 16
/* --- Generic Driver Registration Framework --- */
#define DRIVER_NAME_MAX 32
#define MAX_DRIVERS 16

/* Shared Hardware Context Globals */
extern volatile u32 ticks;
extern int cur_row, cur_col;
extern u8 cur_attr;
extern u16 net_io;
extern u8 net_mac[6];

/* --- Universal Mouse Tracking System Globals --- */
extern u32 cursor_x;
extern u32 cursor_y;
extern volatile u8 mouse_buttons;

/* System Call Parameter Mapping */
struct saved_regs {
    u32 edi, esi, ebp, esp_unused, ebx, edx, ecx, eax;
};

enum driver_type {
    DRV_TYPE_UNKNOWN = 0,
    DRV_TYPE_DISPLAY,
    DRV_TYPE_KEYBOARD,
    DRV_TYPE_NETWORK,
    DRV_TYPE_AUDIO,
    DRV_TYPE_TIMER
};

struct driver {
    char name[DRIVER_NAME_MAX];
    enum driver_type type;
    int (*init)(void);
    void (*shutdown)(void);
};

u32 syscall_dispatch(struct saved_regs *r);

/* Assembly ISR Mapped Functions */
void isr_timer(void);
void isr_keyboard(void);
void isr_fault(void);
void isr_syscall(void);

/* Hardware Port Primitive Operations (Inlined) */
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

static inline void outw(u16 port, u16 value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline u16 inw(u16 port) {
    u16 value;
    __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void io_wait(void) { outb(0x80, 0); }
static inline void irq_on(void) { __asm__ volatile ("sti"); }
static inline void irq_off(void) { __asm__ volatile ("cli"); }
static inline void halt(void) { __asm__ volatile ("hlt"); }

/* Driver System Interfaces */
void serial_init(void);
void clear_screen(void);
void update_cursor(void);
void putc(char c);
void puts(const char *s);
void put_hexn(u32 value, int digits);
void say_error(const char *s);

void idt_init(void);
void pic_init(void);
void pit_init(void);
void wait_ticks(u32 n);

int kbd_read(void);
void speaker_off(void);
void sound_tone(u32 hz, u32 ms);

void anim_start(u32 slot, u8 ease, u32 dur, i32 from, i32 to);
i32 anim_value(u32 slot);

void show_time(void);
int rtl_init(void);
int rtl_link_up(void);
void put_mac(const u8 *mac);
void draw_sqr(u32 x, u32 y);
void boot_splash(void);

void keyboard_irq(void);
int kbd_read(void);

/* Exported Driver Manager Functions */
int driver_register(struct driver *drv);
int drivers_init_all(void);

/* --- New Universal Mouse & Cursor Framework Interfaces --- */
void cursor_move(i32 delta_x, i32 delta_y);
void cursor_driver_bootstrap(void);
void mouse_bootstrap(void);
void mouse_irq_handler(void);
int mouse_init(void);
extern volatile u8 mouse_buttons;
void serial_str(const char *s);
void rtc_now(u8 *out);
int desktop_run(void);
void wait_ticks(u32 n);
void sound_tone(u32 hz, u32 ms);

int kbd_toggle_take(void);
int term_count(void);
const char *term_text(int idx, int *len);
void terminal_exec(const char *cmd);
extern int term_capture;

/* Arrow keys and pointer nudging, used by the desktop */
int kbd_arrow_read(void);
void cursor_nudge(i32 dx, i32 dy);

#endif
