#ifndef AXI_H
#define AXI_H

/* AirOS window API. Apps call the kernel through int 0x80, like the other syscalls.
   A window is a kernel-owned frame: row 0 is the title bar with a [x] close box,
   rows 1..h-1 are the body. Body coordinates start at row 0, col 0 under the title bar. */

#define SYS_WIN_OPEN    14  /* ebx = x | y << 16, ecx = w | h << 16, edx = title; returns handle or AX_FAIL */
#define SYS_WIN_CLOSE   15  /* ebx = handle */
#define SYS_WIN_TEXT    16  /* ebx = handle | attr << 8, ecx = row | col << 16, edx = string */
#define SYS_WIN_CLEAR   17  /* ebx = handle, ecx = attr */
#define SYS_WIN_PRESENT 18  /* read the mouse, then draw every open window and the pointer */
#define SYS_WIN_CLICK   19  /* ebx = handle; returns row | col << 16 of the last body click, or AX_FAIL */
#define SYS_WIN_ALIVE   20  /* ebx = handle; 1 while the user has not closed it */

#define AX_FAIL 0xFFFFFFFFu
#define AX_MAX_BODY_W 40
#define AX_MAX_BODY_H 12
#define AX_CLICK_ROW(v) ((v) & 0xFFFFu)
#define AX_CLICK_COL(v) ((v) >> 16)

static inline unsigned int ax_call(unsigned int n, unsigned int b, unsigned int c, unsigned int d) {
    unsigned int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(n), "b"(b), "c"(c), "d"(d) : "memory");
    return ret;
}

static inline unsigned int ax_open(int x, int y, int w, int h, const char *title) {
    return ax_call(SYS_WIN_OPEN,
                   (unsigned int)x | ((unsigned int)y << 16),
                   (unsigned int)w | ((unsigned int)h << 16),
                   (unsigned int)(unsigned long)title);
}

static inline void ax_close(unsigned int win) {
    ax_call(SYS_WIN_CLOSE, win, 0, 0);
}

static inline void ax_text(unsigned int win, unsigned int row, unsigned int col,
                           unsigned int attr, const char *s) {
    ax_call(SYS_WIN_TEXT, win | (attr << 8), row | (col << 16), (unsigned int)(unsigned long)s);
}

static inline void ax_clear(unsigned int win, unsigned int attr) {
    ax_call(SYS_WIN_CLEAR, win, attr, 0);
}

static inline void ax_present(void) {
    ax_call(SYS_WIN_PRESENT, 0, 0, 0);
}

static inline unsigned int ax_click(unsigned int win) {
    return ax_call(SYS_WIN_CLICK, win, 0, 0);
}

static inline int ax_alive(unsigned int win) {
    return ax_call(SYS_WIN_ALIVE, win, 0, 0) != 0;
}

#endif
