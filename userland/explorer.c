#include "../include/axi.h"
#include "../include/fs.h"
#include "../kernel/textile.h"

#define EX_BG 0x70
#define EX_KEY_ESC 27
#define EX_FIRST_ROW 1
#define EX_LAST_ROW 8
#define EX_BTN_ROW 10
#define EX_TEXT_ROWS 9
#define EX_TEXT_COLS 38
#define EX_MAX_FILES 8

/* No static storage: the app image is loaded from disk and .bss is not zeroed. */
struct ex_state {
    unsigned int win;
    int viewing;
    unsigned int len;
    char name[FS_NAME_MAX + 1];
    char data[FS_FILE_MAX];
};

static const char welcome[] = "Hello from the file explorer.\n";

void app_main(void);

static void app_entry(void) {
    app_main();
    ax_call(SYS_EXIT, 0, 0, 0);
    for (;;)
        ;
}

__attribute__((section(".head"), used))
const unsigned int app_head[2] = { TX_APP_MAGIC, (unsigned int)(unsigned long)app_entry };

static void clear_line(char *line) {
    int i;
    for (i = 0; i < 40; i++)
        line[i] = ' ';
    line[40] = 0;
}

static void make_line(char *line, const char *left, const char *right) {
    int k;
    clear_line(line);
    for (k = 0; left[k] && k < 27; k++)
        line[1 + k] = left[k];
    for (k = 0; right[k] && k < 8; k++)
        line[31 + k] = right[k];
}

static void fmt_dec(char *out, unsigned int n) {
    char tmp[12];
    int d = 0;
    do {
        tmp[d++] = (char)('0' + n % 10);
        n /= 10;
    } while (n);
    while (d)
        *out++ = tmp[--d];
    *out = 0;
}

static void make_name(char *out, unsigned int n) {
    const char *p = "note";
    int k = 0;
    while (p[k]) {
        out[k] = p[k];
        k++;
    }
    out[k++] = (char)('0' + n);
    out[k] = 0;
}

static void draw_list(struct ex_state *s) {
    char line[41];
    char name[FS_NAME_MAX + 1];
    char size[12];
    unsigned int i, bytes;
    int count = 0;

    make_line(line, "Name", "Bytes");
    ax_text(s->win, 0, 0, EX_BG, line);
    for (i = 0; i < EX_MAX_FILES; i++) {
        bytes = fs_list(i, name);
        if (bytes == FS_FAIL) {
            clear_line(line);
        } else {
            fmt_dec(size, bytes);
            make_line(line, name, size);
            count++;
        }
        ax_text(s->win, EX_FIRST_ROW + i, 0, EX_BG, line);
    }
    if (count == 0)
        ax_text(s->win, EX_FIRST_ROW, 1, EX_BG, "No files yet.");
    ax_text(s->win, EX_BTN_ROW, 1, EX_BG, "[ New file ]");
}

static void draw_text(struct ex_state *s) {
    char line[41];
    char size[12];
    unsigned int row, col, pos;

    fmt_dec(size, s->len);
    make_line(line, s->name, size);
    ax_text(s->win, 0, 0, EX_BG, line);
    pos = 0;
    for (row = EX_FIRST_ROW; row < EX_FIRST_ROW + EX_TEXT_ROWS; row++) {
        clear_line(line);
        col = 0;
        while (pos < s->len && col < EX_TEXT_COLS) {
            char c = s->data[pos++];
            if (c == '\n')
                break;
            line[1 + col++] = c;
        }
        ax_text(s->win, row, 0, EX_BG, line);
    }
    ax_text(s->win, EX_BTN_ROW, 1, EX_BG, "[Back]");
    ax_text(s->win, EX_BTN_ROW, 10, EX_BG, "[Delete]");
}

static void open_file(struct ex_state *s, unsigned int index) {
    unsigned int size, h, got;

    size = fs_list(index, s->name);
    if (size == FS_FAIL)
        return;
    h = fs_open(s->name, FS_READ);
    if (h == FS_FAIL)
        return;
    got = fs_read(h, s->data, FS_FILE_MAX);
    fs_close(h);
    s->len = got == FS_FAIL ? 0 : got;
    s->viewing = 1;
}

static void new_file(void) {
    char name[FS_NAME_MAX + 1];
    unsigned int n, h;

    for (n = 1; n <= 9; n++) {
        make_name(name, n);
        h = fs_open(name, FS_READ);
        if (h != FS_FAIL) {
            fs_close(h);
            continue;
        }
        h = fs_open(name, FS_WRITE);
        if (h == FS_FAIL)
            return;
        fs_write(h, welcome, sizeof(welcome) - 1);
        fs_close(h);
        return;
    }
}

static void delete_current(struct ex_state *s) {
    fs_delete(s->name);
    s->viewing = 0;
}

static void handle_click(struct ex_state *s, unsigned int row, unsigned int col) {
    if (s->viewing) {
        if (row == EX_BTN_ROW && col >= 1 && col <= 6)
            s->viewing = 0;
        else if (row == EX_BTN_ROW && col >= 10 && col <= 17)
            delete_current(s);
        return;
    }
    if (row >= EX_FIRST_ROW && row <= EX_LAST_ROW)
        open_file(s, row - EX_FIRST_ROW);
    else if (row == EX_BTN_ROW && col >= 1 && col <= 12)
        new_file();
}

void app_main(void) {
    struct ex_state s;
    unsigned int v;

    s.win = ax_open(8, 2, 40, 12, "Files");
    if (s.win == AX_FAIL)
        return;
    s.viewing = 0;
    s.len = 0;
    s.name[0] = 0;
    for (;;) {
        if (s.viewing)
            draw_text(&s);
        else
            draw_list(&s);
        ax_present();
        if ((int)ax_call(SYS_GETKEY, 0, 0, 0) == EX_KEY_ESC)
            break;
        if (!ax_alive(s.win))
            break;
        v = ax_click(s.win);
        if (v != AX_FAIL)
            handle_click(&s, AX_CLICK_ROW(v), AX_CLICK_COL(v));
        ax_call(SYS_SLEEP, 1, 0, 0);
    }
    ax_close(s.win);
}
