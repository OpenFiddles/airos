/* Textile OS kernel: VGA console, COM1 mirror, PIC, PIT, PS/2 keyboard, CMOS clock, RTL8139 probe,
   tween engine, PC speaker, int 0x80 syscall gate and a flat app loaded at TX_APP_BASE. */

#include "../include/textile.h"

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
    u16 pos = (u16)(cur_row * VGA_COLS + cur_col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (u8)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (u8)(pos >> 8));
}

static void clear_screen(void) {
    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++)
        VGA_MEM[i] = (u16)(' ' | (cur_attr << 8));
    cur_row = 0;
    cur_col = 0;
    update_cursor();
}

static void scroll_up(void) {
    for (int i = 0; i < VGA_COLS * (VGA_ROWS - 1); i++)
        VGA_MEM[i] = VGA_MEM[i + VGA_COLS];
    for (int i = VGA_COLS * (VGA_ROWS - 1); i < VGA_COLS * VGA_ROWS; i++)
        VGA_MEM[i] = (u16)(' ' | (cur_attr << 8));
    cur_row = VGA_ROWS - 1;
}

static void newline(void) {
    cur_col = 0;
    if (++cur_row >= VGA_ROWS)
        scroll_up();
}

void putc(char c) {
    serial_putc(c);
    if (c == '\n') {
        newline();
    } else if (c == '\b') {
        if (cur_col > 0) {
            cur_col--;
            VGA_MEM[cur_row * VGA_COLS + cur_col] = (u16)(' ' | (cur_attr << 8));
        }
    } else {
        VGA_MEM[cur_row * VGA_COLS + cur_col] = (u16)((u8)c | (cur_attr << 8));
        if (++cur_col >= VGA_COLS)
            newline();
    }
    update_cursor();
}

void puts(const char *s) {
    while (*s)
        putc(*s++);
}

void put_hexn(u32 value, int digits) {
    static const char hex[] = "0123456789ABCDEF";
    for (int i = digits - 1; i >= 0; i--)
        putc(hex[(value >> (i * 4)) & 0xF]);
}

static void put_dec(u32 value) {
    char buf[12];
    int n = 0;
    do {
        buf[n++] = (char)('0' + value % 10);
        value /= 10;
    } while (value);
    while (n)
        putc(buf[--n]);
}

void say_error(const char *s) {
    u8 old = cur_attr;
    cur_attr = ATTR_ERR;
    puts(s);
    cur_attr = old;
}

struct idt_entry {
    u16 offset_low;
    u16 selector;
    u8 zero;
    u8 flags;
    u16 offset_high;
} __attribute__((packed));

struct idt_ptr {
    u16 limit;
    u32 base;
} __attribute__((packed));

static struct idt_entry idt[256];

void isr_timer(void);
void isr_keyboard(void);
void isr_fault(void);
void isr_syscall(void);

static void idt_set(int n, void (*handler)(void), u8 flags) {
    u32 addr = (u32)handler;
    idt[n].offset_low = (u16)(addr & 0xFFFF);
    idt[n].selector = 0x08;
    idt[n].zero = 0;
    idt[n].flags = flags;
    idt[n].offset_high = (u16)(addr >> 16);
}

static void idt_init(void) {
    for (int i = 0; i < 32; i++)
        idt_set(i, isr_fault, 0x8E);
    idt_set(0x20, isr_timer, 0x8E);
    idt_set(0x21, isr_keyboard, 0x8E);
    /* Trap gate (0x8F) keeps interrupts enabled while a syscall sleeps. */
    idt_set(0x80, isr_syscall, 0x8F);
    struct idt_ptr ptr = { (u16)(sizeof(idt) - 1), (u32)idt };
    __asm__ volatile ("lidt %0" : : "m"(ptr));
}

static void pic_init(void) {
    outb(0x20, 0x11); io_wait();
    outb(0xA0, 0x11); io_wait();
    outb(0x21, 0x20); io_wait();
    outb(0xA1, 0x28); io_wait();
    outb(0x21, 0x04); io_wait();
    outb(0xA1, 0x02); io_wait();
    outb(0x21, 0x01); io_wait();
    outb(0xA1, 0x01); io_wait();
    outb(0x21, 0xFC);
    outb(0xA1, 0xFF);
}

static void pit_init(void) {
    u16 divisor = 11932; /* 1193182 Hz / 11932 is about 100 interrupts per second */
    outb(0x43, 0x36);
    outb(0x40, (u8)(divisor & 0xFF));
    outb(0x40, (u8)(divisor >> 8));
}

static volatile u32 ticks;

void timer_irq(void) {
    ticks++;
    outb(0x20, 0x20);
}

static void wait_ticks(u32 n) {
    u32 start = ticks;
    while (ticks - start < n)
        halt();
}

int kbd_read(void);

static void speaker_off(void) {
    outb(0x61, inb(0x61) & 0xFC);
}

/* Blocking square-wave tone on PIT channel 2 routed to the PC speaker. */
static void sound_tone(u32 hz, u32 ms) {
    if (hz < 20 || hz > 20000) {
        speaker_off();
        return;
    }
    u32 div = 1193182u / hz;
    outb(0x43, 0xB6);
    outb(0x42, (u8)(div & 0xFF));
    outb(0x42, (u8)(div >> 8));
    outb(0x61, inb(0x61) | 0x03);
    wait_ticks((ms + 5) / 10);
    speaker_off();
}

struct tween {
    u8 used;
    u8 ease;
    u32 dur;
    u32 start;
    i32 from;
    i32 to;
};

static struct tween tweens[ANIM_SLOTS];

/* Easing on a 0..1000 scale in, 0..1000 out. */
static u32 ease_apply(u8 kind, u32 x) {
    if (kind == EASE_LINEAR)
        return x;
    if (kind == EASE_OUT_CUBIC) {
        u32 p = 1000 - x;
        u32 p2 = p * p / 1000;
        u32 p3 = p2 * p / 1000;
        return 1000 - p3;
    }
    return (x * x / 1000) * (3000 - 2 * x) / 1000;
}

static void anim_start(u32 slot, u8 ease, u32 dur, i32 from, i32 to) {
    struct tween *t = &tweens[slot % ANIM_SLOTS];
    t->used = 1;
    t->ease = ease;
    t->dur = dur ? dur : 1;
    t->start = ticks;
    t->from = from;
    t->to = to;
}

static i32 anim_value(u32 slot) {
    struct tween *t = &tweens[slot % ANIM_SLOTS];
    if (!t->used)
        return 0;
    u32 elapsed = ticks - t->start;
    if (elapsed >= t->dur)
        return t->to;
    u32 x = elapsed * 1000 / t->dur;
    i32 e = (i32)ease_apply(t->ease, x);
    return t->from + (t->to - t->from) * e / 1000;
}

static u8 rtc(u8 reg) {
    outb(0x70, reg);
    io_wait();
    return inb(0x71);
}

static u8 from_bcd(u8 v) {
    return (u8)((v & 0x0F) + (v >> 4) * 10);
}

static void put2(u8 v) {
    putc((char)('0' + v / 10));
    putc((char)('0' + v % 10));
}

static void show_time(void) {
    while (rtc(0x0A) & 0x80)
        ;
    u8 sec = rtc(0x00), min = rtc(0x02), hour = rtc(0x04) & 0x7F;
    u8 day = rtc(0x07), mon = rtc(0x08), year = rtc(0x09), status_b = rtc(0x0B);
    if (!(status_b & 0x04)) {
        sec = from_bcd(sec);
        min = from_bcd(min);
        hour = from_bcd(hour);
        day = from_bcd(day);
        mon = from_bcd(mon);
        year = from_bcd(year);
    }
    puts("20");
    put2(year);
    putc('-');
    put2(mon);
    putc('-');
    put2(day);
    putc(' ');
    put2(hour);
    putc(':');
    put2(min);
    putc(':');
    put2(sec);
}

static int str_eq(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static int split(char *s, char **argv, int max) {
    int n = 0;
    while (*s && n < max) {
        while (*s == ' ')
            s++;
        if (!*s)
            break;
        argv[n++] = s;
        while (*s && *s != ' ')
            s++;
        if (*s)
            *s++ = 0;
    }
    return n;
}

static int parse_hex(const char *s, u32 *out) {
    u32 v = 0;
    int digits = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;
    for (; *s; s++) {
        u8 d;
        if (*s >= '0' && *s <= '9')
            d = (u8)(*s - '0');
        else if (*s >= 'a' && *s <= 'f')
            d = (u8)(*s - 'a' + 10);
        else if (*s >= 'A' && *s <= 'F')
            d = (u8)(*s - 'A' + 10);
        else
            return 0;
        v = (v << 4) | d;
        digits++;
    }
    if (!digits)
        return 0;
    *out = v;
    return 1;
}

static int parse_dec(const char *s, u32 *out) {
    u32 v = 0;
    int digits = 0;
    for (; *s; s++) {
        if (*s < '0' || *s > '9')
            return 0;
        v = v * 10 + (u32)(*s - '0');
        if (++digits > 4)
            return 0;
    }
    if (!digits)
        return 0;
    *out = v;
    return 1;
}

static u8 net_slot;
static u16 net_io;
static u8 net_mac[6];

static u32 pci_read(u8 slot, u8 off) {
    outl(PCI_ADDR, 0x80000000u | ((u32)slot << 11) | (off & 0xFCu));
    return inl(PCI_DATA);
}

static int rtl_probe(void) {
    for (u8 slot = 0; slot < PCI_SLOT_MAX; slot++) {
        u32 id = pci_read(slot, 0x00);
        if ((id & 0xFFFFu) == RTL_VENDOR && (id >> 16) == RTL_DEVICE) {
            net_slot = slot;
            return 1;
        }
    }
    return 0;
}

static int rtl_init(void) {
    u32 cmd;
    if (!rtl_probe())
        return 0;
    outl(PCI_ADDR, 0x80000000u | ((u32)net_slot << 11) | 0x04u);
    cmd = inl(PCI_DATA);
    outl(PCI_DATA, (cmd & 0x0000FFFFu) | 0x05u);
    net_io = (u16)(pci_read(net_slot, 0x10) & 0xFFFCu);
    outb((u16)(net_io + 0x52), 0x00);
    outb((u16)(net_io + 0x37), 0x10);
    for (u32 i = 0; i < 1000000 && (inb((u16)(net_io + 0x37)) & 0x10); i++)
        ;
    if (inb((u16)(net_io + 0x37)) & 0x10)
        return 0;
    for (int i = 0; i < 6; i++)
        net_mac[i] = inb((u16)(net_io + i));
    return 1;
}

static int rtl_link_up(void) {
    return !(inb((u16)(net_io + 0x58)) & 0x04);
}

static void put_mac(const u8 *mac) {
    for (int i = 0; i < 6; i++) {
        put_hexn(mac[i], 2);
        if (i < 5)
            putc(':');
    }
}

void draw_sqr(u32 x, u32 y) {
    u16 cell = (u16)(0xDB | (ATTR_SQR << 8));
    for (u32 dy = 0; dy < 2; dy++)
        for (u32 dx = 0; dx < 2; dx++)
            VGA_MEM[(y + dy) * VGA_COLS + x + dx] = cell;
}


static int slen(const char *s) {
    int n = 0;
    while (s[n])
        n++;
    return n;
}

static const char *const saturn[5] = {
    "   PPPPPPPPPP   ",
    " PPPPPPPPPPPPPP ",
    "=========BBBBBBBBBBBBBB=========",
    " PPPPPPPPPPPPPP ",
    "   PPPPPPPPPP   "
};

static void splash_cell(int row, int col, u8 glyph, u8 fg, u8 bg) {
    VGA_MEM[row * VGA_COLS + col] = (u16)(glyph | (((bg << 4) | fg) << 8));
}

static void splash_text(int row, const char *s, u8 fg, u8 bg) {
    int len = slen(s), col = (VGA_COLS - len) / 2;
    for (int i = 0; i < len; i++)
        splash_cell(row, col + i, (u8)s[i], fg, bg);
}

static void splash_frame(u8 bg) {
    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++)
        VGA_MEM[i] = (u16)(' ' | (bg << 12));
    for (int r = 0; r < 5; r++) {
        int len = slen(saturn[r]), col0 = (VGA_COLS - len) / 2;
        for (int i = 0; i < len; i++) {
            char c = saturn[r][i];
            if (c == 'P')
                splash_cell(7 + r, col0 + i, 0xDB, 0x0E, bg);
            else if (c == 'B')
                splash_cell(7 + r, col0 + i, 0xB2, 0x06, bg);
            else if (c == '=')
                splash_cell(7 + r, col0 + i, 0xCD, 0x0B, bg);
        }
    }
    splash_text(14, "OpenFiddles", 0x0F, bg);
    splash_text(16, "OpenAir", 0x07, bg);
}

static void boot_splash(void) {
    static const u8 pulse[12] = {0, 0, 1, 1, 1, 3, 3, 3, 1, 1, 0, 0};
    for (int cycle = 0; cycle < 4; cycle++)
        for (int f = 0; f < 12; f++) {
            splash_frame(pulse[f]);
            wait_ticks(10);
        }
}

struct saved_regs {
    u32 edi, esi, ebp, esp_unused, ebx, edx, ecx, eax;
};

void syscall_dispatch(struct saved_regs *r) {
    switch (r->eax) {
    case SYS_PUTC:
        putc((char)r->ebx);
        break;
    case SYS_PUTAT:
        if (r->ebx < VGA_ROWS && r->ecx < VGA_COLS)
            VGA_MEM[r->ebx * VGA_COLS + r->ecx] = (u16)r->edx;
        break;
    case SYS_CLEAR:
        clear_screen();
        break;
    case SYS_TICKS:
        r->eax = ticks;
        break;
    case SYS_SLEEP:
        wait_ticks(r->ebx);
        break;
    case SYS_TONE:
        sound_tone(r->ebx, r->ecx);
        break;
    case SYS_ANIM_START:
        anim_start(r->ebx & 0xFF, (u8)((r->ebx >> 8) & 0xFF), (r->ebx >> 16) & 0xFFFF,
                   (i32)r->ecx, (i32)r->edx);
        break;
    case SYS_ANIM_VALUE:
        r->eax = (u32)anim_value(r->ebx);
        break;
    case SYS_GETKEY:
        r->eax = (u32)kbd_read();
        break;
    case SYS_PRINT:
        puts((const char *)r->ebx);
        break;
    default:
        r->eax = (u32)-1;
        break;
    }
}

static int run_app(void) {
    const u32 *head = (const u32 *)TX_APP_BASE;
    if (head[0] != TX_APP_MAGIC)
        return 0;
    ((void (*)(void))head[1])();
    return 1;
}

static void run_command(char *input) {
    char *argv[8];
    int argc = split(input, argv, 8);
    if (argc == 0)
        return;

    if (str_eq(argv[0], "help")) {
        puts("Commands:\n");
        puts("  help           show this list\n");
        puts("  clear          clear the screen\n");
        puts("  echo TEXT      print TEXT\n");
        puts("  time           read the clock chip (CMOS RTC, ports 0x70/0x71)\n");
        puts("  uptime         seconds since boot (PIT timer, IRQ0)\n");
        puts("  in PORT        read one byte from an I/O port, e.g. in 60\n");
        puts("  out PORT VAL   write one byte to an I/O port, e.g. out 80 0\n");
        puts("  reboot         reset through the keyboard controller\n");
        puts("  net            show the RTL8139 network card and its MAC\n");
        puts("  sqr X Y        draw a 2x2 square at column X, row Y\n");
        puts("  beep HZ MS     play a tone on the PC speaker\n");
        puts("  run            run the intro app again\n");
    } else if (str_eq(argv[0], "clear")) {
        clear_screen();
    } else if (str_eq(argv[0], "echo")) {
        for (int i = 1; i < argc; i++) {
            if (i > 1)
                putc(' ');
            puts(argv[i]);
        }
        putc('\n');
    } else if (str_eq(argv[0], "time")) {
        show_time();
        putc('\n');
    } else if (str_eq(argv[0], "uptime")) {
        puts("up ");
        put_dec(ticks / 100);
        puts(" s\n");
    } else if (str_eq(argv[0], "in")) {
        u32 port;
        if (argc < 2 || !parse_hex(argv[1], &port) || port > 0xFFFF) {
            say_error("usage: in PORT   (hex, e.g. in 60)\n");
            return;
        }
        u8 v = inb((u16)port);
        puts("port 0x");
        put_hexn(port, 4);
        puts(" = 0x");
        put_hexn(v, 2);
        putc('\n');
    } else if (str_eq(argv[0], "out")) {
        u32 port, value;
        if (argc < 3 || !parse_hex(argv[1], &port) || !parse_hex(argv[2], &value) ||
            port > 0xFFFF || value > 0xFF) {
            say_error("usage: out PORT VALUE   (hex, e.g. out 80 0)\n");
            return;
        }
        outb((u16)port, (u8)value);
        puts("wrote 0x");
        put_hexn(value, 2);
        puts(" to port 0x");
        put_hexn(port, 4);
        putc('\n');
    } else if (str_eq(argv[0], "reboot")) {
        puts("rebooting...\n");
        outb(0x64, 0xFE);
        for (;;)
            halt();
    } else if (str_eq(argv[0], "net")) {
        if (!rtl_init()) {
            say_error("no RTL8139 network card found\n");
        } else {
            puts("rtl8139 at io 0x");
            put_hexn(net_io, 4);
            puts("  mac ");
            put_mac(net_mac);
            puts(rtl_link_up() ? "  link up\n" : "  link down\n");
        }
    } else if (str_eq(argv[0], "sqr")) {
        u32 x, y;
        if (argc < 3 || !parse_dec(argv[1], &x) || !parse_dec(argv[2], &y) ||
            x > VGA_COLS - 2 || y > VGA_ROWS - 2) {
            say_error("usage: sqr X Y   (X 0-78, Y 0-23)\n");
            return;
        }
        draw_sqr(x, y);
        puts("square at ");
        put_dec(x);
        putc(' ');
        put_dec(y);
        putc('\n');
    } else if (str_eq(argv[0], "beep")) {
        u32 hz, ms;
        if (argc < 3 || !parse_dec(argv[1], &hz) || !parse_dec(argv[2], &ms) ||
            hz < 20 || hz > 20000) {
            say_error("usage: beep HZ MS   (e.g. beep 523 200)\n");
            return;
        }
        puts("beep ");
        put_dec(hz);
        puts(" Hz\n");
        sound_tone(hz, ms);
    } else if (str_eq(argv[0], "run")) {
        if (!run_app()) {
            say_error("no app installed\n");
            return;
        }
        clear_screen();
    } else {
        say_error("unknown command: ");
        puts(argv[0]);
        putc('\n');
    }
}

void kmain(void) {
    char line[80];
    int len = 0;

    serial_init();
    clear_screen();
    idt_init();
    pic_init();
    pit_init();
    irq_on();
    boot_splash();
    run_app();
    clear_screen();

    cur_attr = ATTR_OK;
    puts("Textile OS 0.3 kernel\n");
    cur_attr = ATTR_TEXT;
    puts("Keyboard, timer, clock, speaker, animation and I/O ports are live. Type help.\n");
    puts("textile> ");

    for (;;) {
        int c = kbd_read();
        if (c < 0) {
            halt();
            continue;
        }
        if (c == '\n') {
            putc('\n');
            line[len] = 0;
            run_command(line);
            len = 0;
            puts("textile> ");
        } else if (c == '\b') {
            if (len > 0) {
                len--;
                putc('\b');
            }
        } else if (len < (int)sizeof(line) - 1) {
            line[len++] = (char)c;
            putc((char)c);
        }
    }
}
