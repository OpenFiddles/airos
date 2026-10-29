#include "../include/axi.h"
#include "../kernel/textile.h"

#define WIN_BG 0x70
#define WIN_KEY_ESC 27

void app_main(void);

static void app_entry(void) {
    app_main();
    ax_call(SYS_EXIT, 0, 0, 0);
    for (;;)
        ;
}

__attribute__((section(".head"), used))
const unsigned int app_head[2] = { TX_APP_MAGIC, (unsigned int)(unsigned long)app_entry };

static const char blank[] = "                        ";

static void format_clicks(char *out, unsigned int n) {
    const char *label = "Clicks: ";
    int k = 0;
    while (label[k]) {
        out[k] = label[k];
        k++;
    }
    char digits[12];
    int d = 0;
    do {
        digits[d++] = (char)('0' + n % 10);
        n /= 10;
    } while (n);
    while (d)
        out[k++] = digits[--d];
    out[k] = 0;
}

void app_main(void) {
    unsigned int win = ax_open(10, 3, 40, 12, "Window API demo");
    if (win == AX_FAIL)
        return;

    ax_clear(win, WIN_BG);
    ax_text(win, 1, 2, WIN_BG, "Hello from a window app.");
    ax_text(win, 3, 2, WIN_BG, "Click inside this window to count.");
    ax_text(win, 5, 2, WIN_BG, "Press Esc to close it.");

    unsigned int clicks = 0;
    char line[24];
    for (;;) {
        ax_present();

        if ((int)ax_call(SYS_GETKEY, 0, 0, 0) == WIN_KEY_ESC)
            break;
        if (!ax_alive(win))
            break;

        if (ax_click(win) != AX_FAIL) {
            clicks++;
            format_clicks(line, clicks);
            ax_text(win, 7, 2, WIN_BG, blank);
            ax_text(win, 7, 2, WIN_BG, line);
        }
        ax_call(SYS_SLEEP, 1, 0, 0);
    }
    ax_close(win);
}
