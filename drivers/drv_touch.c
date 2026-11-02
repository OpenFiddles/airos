#include "../include/drv.h"
#include "../include/hw.h"
#include "../include/drv_touch.h"

/* Polled touch input. The page writes ASCII lines "T x y d\n" to COM2:
   x and y in 0..65535, d is 1 while a finger is down and 0 on lift.
   No IRQ is used, so the PIC mask is unchanged. */

#define TOUCH_PORT 0x2F8
#define REG_DATA 0
#define REG_IER 1
#define REG_FCR 2
#define REG_LCR 3
#define REG_MCR 4
#define REG_LSR 5
#define LSR_DATA_READY 0x01
#define POLL_BYTES_MAX 64

static struct touch_ev ring[TOUCH_RING];
static unsigned ring_head;
static unsigned ring_tail;
static char line[TOUCH_LINE_MAX];
static unsigned line_len;
static unsigned line_overflow;
static int touch_ready;

static int ring_put(struct touch_ev ev) {
    unsigned next = (ring_head + 1) % TOUCH_RING;
    if (next == ring_tail)
        return 0;
    ring[ring_head] = ev;
    ring_head = next;
    return 1;
}

static int read_num(const char *s, unsigned n, unsigned *i, unsigned *out) {
    unsigned v = 0;
    unsigned digits = 0;
    while (*i < n && s[*i] >= '0' && s[*i] <= '9') {
        v = v * 10 + (unsigned)(s[*i] - '0');
        if (v > 65535)
            return 0;
        (*i)++;
        digits++;
    }
    if (!digits)
        return 0;
    *out = v;
    return 1;
}

static int parse_line(const char *s, unsigned n, struct touch_ev *ev) {
    unsigned i = 2;
    unsigned v;

    if (n < 7 || s[0] != 'T' || s[1] != ' ')
        return 0;
    if (!read_num(s, n, &i, &v))
        return 0;
    ev->x = (unsigned short)v;
    if (i >= n || s[i] != ' ')
        return 0;
    i++;
    if (!read_num(s, n, &i, &v))
        return 0;
    ev->y = (unsigned short)v;
    if (i + 2 != n || s[i] != ' ' || (s[i + 1] != '0' && s[i + 1] != '1'))
        return 0;
    ev->down = (unsigned char)(s[i + 1] == '1');
    return 1;
}

int touch_poll(void) {
    int queued = 0;
    int budget = POLL_BYTES_MAX;

    if (!touch_ready)
        return 0;
    while (budget-- > 0 && (hw_inb(TOUCH_PORT + REG_LSR) & LSR_DATA_READY)) {
        char c = (char)hw_inb(TOUCH_PORT + REG_DATA);
        if (c == '\n') {
            struct touch_ev ev;
            if (line_len && line[line_len - 1] == '\r')
                line_len--;
            if (!line_overflow && parse_line(line, line_len, &ev) && ring_put(ev))
                queued++;
            line_len = 0;
            line_overflow = 0;
        } else if (line_len < TOUCH_LINE_MAX) {
            line[line_len++] = c;
        } else {
            line_overflow = 1;
        }
    }
    return queued;
}

int touch_next(struct touch_ev *out) {
    if (ring_tail == ring_head)
        return 0;
    *out = ring[ring_tail];
    ring_tail = (ring_tail + 1) % TOUCH_RING;
    return 1;
}

static int touch_init(void) {
    hw_outb(TOUCH_PORT + REG_IER, 0x00);
    hw_outb(TOUCH_PORT + REG_LCR, 0x80);
    hw_outb(TOUCH_PORT + REG_DATA, 0x0C);
    hw_outb(TOUCH_PORT + REG_IER, 0x00);
    hw_outb(TOUCH_PORT + REG_LCR, 0x03);
    hw_outb(TOUCH_PORT + REG_FCR, 0xC7);
    hw_outb(TOUCH_PORT + REG_MCR, 0x0B);
    ring_head = 0;
    ring_tail = 0;
    line_len = 0;
    line_overflow = 0;
    touch_ready = 1;
    return 0;
}

static const struct drv_ops drv_touch_ops = { touch_init, 0, 0 };

struct drv drv_touch_device = { "touch", DRV_CLASS_INPUT, &drv_touch_ops, DRV_NO_IRQ, 0 };
