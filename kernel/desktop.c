#include "kernel.h"

#define TASK_ROW (VGA_ROWS - 1)
#define BG_CHAR 0xB0
#define BG_ATTR 0x13
#define BODY_ATTR 0x70
#define TITLE_ON 0x1F
#define TITLE_OFF 0x87
#define TASK_ATTR 0x70
#define BTN_OPEN 0x87
#define BTN_TOP 0x1F
#define ICON_ATTR 0x70
#define KEY_ATTR 0x4F
#define PIANO_KEYS 8
#define PIANO_FLASH 12
#define FRAME_TICKS 2
#define CURSOR_ATTR 0x0F

enum { WIN_HELLO, WIN_PIANO, WIN_CLOCK, WIN_TERM, WIN_COUNT };

struct window {
    int open;
    i32 x, y, w, h;
    const char *title;
};

static struct window wins[WIN_COUNT] = {
    {0, 22, 3, 38, 9, "Hello"},
    {0, 12, 12, 40, 7, "Piano"},
    {0, 60, 4, 18, 5, "Clock"},
    {0, 4, 2, 60, 17, "Terminal"},
};

static const char *const icon_label[WIN_COUNT] = {"Hello", "Piano", "Clock", "Term"};
static const char *const log_name[WIN_COUNT] = {"hello", "piano", "clock", "terminal"};
static const i32 icon_row[WIN_COUNT] = {2, 4, 6, 8};
static const char piano_keys[PIANO_KEYS + 1] = "asdfghjk";
static const char piano_notes[PIANO_KEYS + 1] = "CDEFGABC";
static const u32 piano_hz[PIANO_KEYS] = {262, 294, 330, 349, 392, 440, 494, 523};

static u16 back[VGA_ROWS * VGA_COLS];
static int z[WIN_COUNT] = {WIN_HELLO, WIN_PIANO, WIN_CLOCK, WIN_TERM};
static int drag_win = -1;
static i32 drag_dx, drag_dy;
static u8 prev_buttons;
static int piano_last = -1;
static int piano_flash;

static void cell(int r, int c, u8 ch, u8 attr) {
    if (r < 0 || r >= VGA_ROWS || c < 0 || c >= VGA_COLS)
        return;
    back[r * VGA_COLS + c] = (u16)(ch | (attr << 8));
}

static void text(int r, int c, const char *s, u8 attr, int maxw) {
    for (int i = 0; s[i] && i < maxw; i++)
        cell(r, c + i, (u8)s[i], attr);
}

static void fill(int r, int c, int h, int w, u8 ch, u8 attr) {
    for (int dr = 0; dr < h; dr++)
        for (int dc = 0; dc < w; dc++)
            cell(r + dr, c + dc, ch, attr);
}

static void put2(int r, int c, u8 v, u8 attr) {
    cell(r, c, (u8)('0' + v / 10), attr);
    cell(r, c + 1, (u8)('0' + v % 10), attr);
}

static void draw_hms(int r, int c, u8 attr, const u8 *t) {
    put2(r, c, t[2], attr);
    cell(r, c + 2, ':', attr);
    put2(r, c + 3, t[1], attr);
    cell(r, c + 5, ':', attr);
    put2(r, c + 6, t[0], attr);
}

static i32 clampi(i32 v, i32 lo, i32 hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static int num_str(char *out, i32 v) {
    char tmp[12];
    int n = 0, k = 0;
    if (v < 0) { out[k++] = '-'; v = -v; }
    do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) out[k++] = tmp[--n];
    return k;
}

static void log_point(const char *tag, i32 a, i32 b) {
    char buf[64];
    int n = 0;
    for (const char *p = tag; *p && n < 30; p++) buf[n++] = *p;
    buf[n++] = ' ';
    n += num_str(buf + n, a);
    buf[n++] = ' ';
    n += num_str(buf + n, b);
    buf[n++] = '\n';
    buf[n] = 0;
    serial_str(buf);
}

static int btn_col(int i) { return 8 + i * 10; }

static void raise_win(int k) {
    int i = 0;
    while (i < WIN_COUNT && z[i] != k) i++;
    for (; i < WIN_COUNT - 1; i++) z[i] = z[i + 1];
    z[WIN_COUNT - 1] = k;
}

static int top_win(void) {
    for (int i = WIN_COUNT - 1; i >= 0; i--)
        if (wins[z[i]].open) return z[i];
    return -1;
}

static int win_contains(const struct window *w, i32 mx, i32 my) {
    return mx >= w->x && mx < w->x + w->w && my >= w->y && my < w->y + w->h;
}

static int window_at(i32 mx, i32 my) {
    for (int i = WIN_COUNT - 1; i >= 0; i--) {
        int k = z[i];
        if (wins[k].open && win_contains(&wins[k], mx, my)) return k;
    }
    return -1;
}

static void open_win(int k) {
    wins[k].open = 1;
    raise_win(k);
    serial_str("desktop: open ");
    serial_str(log_name[k]);
    serial_str("\n");
}

static void piano_play(int i) {
    piano_last = i;
    piano_flash = PIANO_FLASH;
    log_point("piano: key", i, 0);
    sound_tone(piano_hz[i], 120);
}

#define TERM_IN_MAX 50
static char term_in[TERM_IN_MAX + 1];
static int term_in_len;

static void draw_terminal(const struct window *w) {
    int rows = w->h - 2;
    int total = term_count();
    int first = total - rows;
    if (first < 0)
        first = 0;
    for (int r = 0; r < rows && first + r < total; r++) {
        int len;
        const char *s = term_text(first + r, &len);
        for (int c = 0; c < len && c < w->w - 4; c++)
            cell(w->y + 1 + r, w->x + 2 + c, (u8)s[c], BODY_ATTR);
    }
    int py = w->y + w->h - 1;
    term_in[term_in_len] = 0;
    text(py, w->x + 2, "> ", BODY_ATTR, 2);
    text(py, w->x + 4, term_in, BODY_ATTR, w->w - 6);
    cell(py, w->x + 4 + term_in_len, '_', BODY_ATTR);
}

static void draw_content(int k) {
    const struct window *w = &wins[k];
    if (k == WIN_TERM) {
        draw_terminal(w);
        return;
    }
    if (k == WIN_HELLO) {
        text(w->y + 2, w->x + 2, "Welcome to AirOS.", BODY_ATTR, w->w - 4);
        text(w->y + 3, w->x + 2, "Click an icon on the left", BODY_ATTR, w->w - 4);
        text(w->y + 4, w->x + 2, "or a button on the taskbar.", BODY_ATTR, w->w - 4);
        text(w->y + 6, w->x + 2, "Ctrl+Alt+D opens developer mode.", BODY_ATTR, w->w - 4);
        text(w->y + 7, w->x + 2, "Arrow keys move the pointer.", BODY_ATTR, w->w - 4);
    } else if (k == WIN_PIANO) {
        text(w->y + 2, w->x + 2, "Click or type a s d f g h j k", BODY_ATTR, w->w - 4);
        for (int i = 0; i < PIANO_KEYS; i++) {
            u8 a = (piano_flash > 0 && piano_last == i) ? KEY_ATTR : BODY_ATTR;
            cell(w->y + 3, w->x + 2 + i * 4, (u8)piano_keys[i], a);
            cell(w->y + 4, w->x + 2 + i * 4, (u8)piano_notes[i], a);
        }
        if (piano_last >= 0) {
            text(w->y + 5, w->x + 2, "Last key:", BODY_ATTR, 9);
            cell(w->y + 5, w->x + 12, (u8)piano_keys[piano_last], BODY_ATTR);
        } else {
            text(w->y + 5, w->x + 2, "Nothing played yet", BODY_ATTR, w->w - 4);
        }
    } else {
        u8 t[6];
        int c = w->x + (w->w - 8) / 2;
        int dc = w->x + (w->w - 10) / 2;
        rtc_now(t);
        draw_hms(w->y + 2, c, BODY_ATTR, t);
        cell(w->y + 3, dc, '2', BODY_ATTR);
        cell(w->y + 3, dc + 1, '0', BODY_ATTR);
        put2(w->y + 3, dc + 2, t[5], BODY_ATTR);
        cell(w->y + 3, dc + 4, '-', BODY_ATTR);
        put2(w->y + 3, dc + 5, t[4], BODY_ATTR);
        cell(w->y + 3, dc + 7, '-', BODY_ATTR);
        put2(w->y + 3, dc + 8, t[3], BODY_ATTR);
    }
}

static void draw_window(int k) {
    const struct window *w = &wins[k];
    u8 ta = (top_win() == k) ? TITLE_ON : TITLE_OFF;
    fill(w->y + 1, w->x, w->h - 1, w->w, ' ', BODY_ATTR);
    fill(w->y, w->x, 1, w->w, ' ', ta);
    text(w->y, w->x + 1, wins[k].title, ta, w->w - 6);
    text(w->y, w->x + w->w - 4, "[x]", ta, 3);
    draw_content(k);
}

static void draw_taskbar(void) {
    u8 t[6];
    int top = top_win();
    fill(TASK_ROW, 0, 1, VGA_COLS, ' ', TASK_ATTR);
    text(TASK_ROW, 1, "AirOS", TASK_ATTR, 5);
    for (int i = 0; i < WIN_COUNT; i++) {
        u8 a = !wins[i].open ? TASK_ATTR : (top == i ? BTN_TOP : BTN_OPEN);
        fill(TASK_ROW, btn_col(i), 1, 9, ' ', a);
        text(TASK_ROW, btn_col(i) + 1, icon_label[i], a, 7);
    }
    text(TASK_ROW, VGA_COLS - 30, "Ctrl+Alt+D: dev", TASK_ATTR, 15);
    rtc_now(t);
    draw_hms(TASK_ROW, VGA_COLS - 9, TASK_ATTR, t);
}

static void draw_cursor(void) {
    i32 r = (i32)cursor_y, c = (i32)cursor_x;
    if (r < 0 || r >= VGA_ROWS || c < 0 || c >= VGA_COLS) return;
    back[r * VGA_COLS + c] = (u16)(0xDB | (CURSOR_ATTR << 8));
}

static void draw_frame(void) {
    fill(0, 0, TASK_ROW, VGA_COLS, BG_CHAR, BG_ATTR);
    text(0, 2, "AirOS home", 0x1E, 10);
    for (int i = 0; i < WIN_COUNT; i++) {
        fill(icon_row[i], 2, 1, 9, ' ', ICON_ATTR);
        text(icon_row[i], 3, icon_label[i], ICON_ATTR, 7);
    }
    for (int i = 0; i < WIN_COUNT; i++)
        if (wins[z[i]].open) draw_window(z[i]);
    draw_taskbar();
    draw_cursor();
    for (int i = 0; i < VGA_ROWS * VGA_COLS; i++)
        VGA_MEM[i] = back[i];
}

static void click(i32 mx, i32 my) {
    log_point("desktop: click", mx, my);
    if (my == TASK_ROW) {
        for (int i = 0; i < WIN_COUNT; i++)
            if (mx >= btn_col(i) && mx < btn_col(i) + 9) { open_win(i); return; }
        return;
    }
    int k = window_at(mx, my);
    if (k >= 0) {
        struct window *w = &wins[k];
        if (my == w->y && mx >= w->x + w->w - 4 && mx <= w->x + w->w - 2) {
            w->open = 0; return;
        }
        raise_win(k);
        if (my == w->y) {
            drag_win = k;
            drag_dx = mx - w->x;
            drag_dy = my - w->y;
            return;
        }
        if (k == WIN_PIANO && my == w->y + 3) {
            i32 rx = mx - (w->x + 2);
            if (rx >= 0 && rx % 4 == 0 && rx / 4 < PIANO_KEYS)
                piano_play(rx / 4);
        }
        return;
    }
    for (int i = 0; i < WIN_COUNT; i++)
        if (mx >= 2 && mx <= 10 && my == icon_row[i]) { open_win(i); return; }
}

static void handle_mouse(void) {
    i32 mx = (i32)cursor_x, my = (i32)cursor_y;
    u8 b = mouse_buttons;
    if ((b & 1) && !(prev_buttons & 1)) click(mx, my);
    if (!(b & 1)) { drag_win = -1; }
    else if (drag_win >= 0) {
        struct window *w = &wins[drag_win];
        w->x = clampi(mx - drag_dx, 0, VGA_COLS - w->w);
        w->y = clampi(my - drag_dy, 0, TASK_ROW - w->h);
    }
    prev_buttons = b;
}

static void term_key(int c) {
    if (c == '\n') {
        term_in[term_in_len] = 0;
        terminal_exec(term_in);
        term_in_len = 0;
        return;
    }
    if (c == '\b') {
        if (term_in_len > 0)
            term_in_len--;
        return;
    }
    if (c >= 32 && c < 127 && term_in_len < TERM_IN_MAX)
        term_in[term_in_len++] = (char)c;
}

static void handle_key(int c) {
    if (top_win() == WIN_TERM) {
        term_key(c);
        return;
    }
    if (top_win() != WIN_PIANO) return;
    for (int i = 0; i < PIANO_KEYS; i++)
        if (piano_keys[i] == c) { piano_play(i); return; }
}

int desktop_run(void) {
    serial_str("desktop: start\n");
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
    prev_buttons = mouse_buttons;
    drag_win = -1;
    for (;;) {
        if (kbd_toggle_take())
            break;
        int c;
        while ((c = kbd_read()) >= 0)
            handle_key(c);
        int a;
        while ((a = kbd_arrow_read()) >= 0) {
            i32 dx = 0, dy = 0;
            if (a == ARROW_UP) dy = -1;
            else if (a == ARROW_DOWN) dy = 1;
            else if (a == ARROW_LEFT) dx = -1;
            else dx = 1;
            cursor_nudge(dx, dy);
        }
        handle_mouse();
        if (piano_flash > 0) piano_flash--;
        draw_frame();
        wait_ticks(FRAME_TICKS);
    }
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x0E);
    serial_str("desktop: exit\n");
    return 1;
}
