#ifndef FLAREOS_SDK_H
#define FLAREOS_SDK_H

/*
 * FlareOS app SDK, version 1.
 * Apps are flat binaries linked at FLARE_APP_BASE (userland/app.ld), capped at
 * FLARE_APP_MAX_BYTES, running in ring 3. They call the kernel with int 0x80.
 * The FLARE_SYS_* numbers must match kernel/textile.h.
 */

typedef unsigned char flare_u8;
typedef unsigned int flare_u32;
typedef int flare_i32;

#define FLARE_APP_MAGIC 0x54505041u
#define FLARE_APP_BASE 0x20000u
#define FLARE_APP_MAX_BYTES 4096u

#define FLARE_SYS_PUTC 1u
#define FLARE_SYS_PUTAT 2u
#define FLARE_SYS_CLEAR 3u
#define FLARE_SYS_TICKS 4u
#define FLARE_SYS_SLEEP 5u
#define FLARE_SYS_TONE 6u
#define FLARE_SYS_ANIM_START 7u
#define FLARE_SYS_ANIM_VALUE 8u
#define FLARE_SYS_GETKEY 9u
#define FLARE_SYS_PRINT 10u
#define FLARE_SYS_EXIT 11u
#define FLARE_SYS_GETARROW 12u

#define FLARE_EASE_LINEAR 0u
#define FLARE_EASE_OUT_CUBIC 1u
#define FLARE_EASE_IN_OUT 2u

#define FLARE_ARROW_UP 1
#define FLARE_ARROW_DOWN 2
#define FLARE_ARROW_LEFT 3
#define FLARE_ARROW_RIGHT 4

#define FLARE_ANIM_SLOTS 16u

#define FLARE_SYS_POINTER 29u
#define FLARE_SYS_SCREEN 30u
#define FLARE_SYS_FS_OPEN 21u
#define FLARE_SYS_FS_READ 22u
#define FLARE_SYS_FS_WRITE 23u
#define FLARE_SYS_FS_CLOSE 24u
#define FLARE_SYS_FS_DELETE 25u
#define FLARE_SYS_FS_LIST 26u
#define FLARE_SYS_FS_SIZE 27u
#define FLARE_SYS_FS_SEEK 28u

#define FLARE_FS_READ 0u
#define FLARE_FS_WRITE 1u
#define FLARE_FS_APPEND 2u
#define FLARE_FS_FAIL 0xFFFFFFFFu
#define FLARE_FS_NAME_MAX 12u
#define FLARE_FS_FILE_MAX 512u

static inline flare_u32 flare_call(flare_u32 n, flare_u32 b, flare_u32 c, flare_u32 d) {
    flare_u32 ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(n), "b"(b), "c"(c), "d"(d) : "memory");
    return ret;
}

static inline void flare_putc(char ch) {
    flare_call(FLARE_SYS_PUTC, (flare_u8)ch, 0, 0);
}

static inline void flare_put_cell(flare_u32 row, flare_u32 col, flare_u8 ch, flare_u8 attr) {
    flare_call(FLARE_SYS_PUTAT, row, col, (flare_u32)ch | ((flare_u32)attr << 8));
}

static inline void flare_clear(void) {
    flare_call(FLARE_SYS_CLEAR, 0, 0, 0);
}

static inline flare_u32 flare_ticks(void) {
    return flare_call(FLARE_SYS_TICKS, 0, 0, 0);
}

static inline void flare_sleep(flare_u32 ticks) {
    flare_call(FLARE_SYS_SLEEP, ticks, 0, 0);
}

static inline void flare_tone(flare_u32 hz, flare_u32 ms) {
    flare_call(FLARE_SYS_TONE, hz, ms, 0);
}

static inline void flare_anim_start(flare_u32 slot, flare_u32 ease, flare_u32 ticks,
                                    flare_i32 from, flare_i32 to) {
    flare_call(FLARE_SYS_ANIM_START,
               (slot & 0xFFu) | ((ease & 0xFFu) << 8) | ((ticks & 0xFFFFu) << 16),
               (flare_u32)from, (flare_u32)to);
}

static inline flare_i32 flare_anim_value(flare_u32 slot) {
    return (flare_i32)flare_call(FLARE_SYS_ANIM_VALUE, slot, 0, 0);
}

static inline flare_i32 flare_getkey(void) {
    return (flare_i32)flare_call(FLARE_SYS_GETKEY, 0, 0, 0);
}

static inline flare_i32 flare_get_arrow(void) {
    return (flare_i32)flare_call(FLARE_SYS_GETARROW, 0, 0, 0);
}

static inline void flare_print(const char *s) {
    flare_call(FLARE_SYS_PRINT, (flare_u32)(unsigned long)s, 0, 0);
}

static inline void flare_exit(void) {
    flare_call(FLARE_SYS_EXIT, 0, 0, 0);
}

/* Pointer state: x and y in text cells (0-79, 0-24), buttons bit 0 = left. */
typedef struct { flare_u8 x, y, buttons; } flare_pointer_t;

static inline flare_pointer_t flare_pointer(void) {
    flare_u32 v = flare_call(FLARE_SYS_POINTER, 0, 0, 0);
    flare_pointer_t p;
    p.x = (flare_u8)(v & 0xFFu);
    p.y = (flare_u8)((v >> 8) & 0xFFu);
    p.buttons = (flare_u8)((v >> 16) & 0x07u);
    return p;
}

/* Screen size in text cells. Today the kernel is 80x25 VGA text, so this returns 80 and 25. */
static inline void flare_screen(flare_u32 *cols, flare_u32 *rows) {
    flare_u32 v = flare_call(FLARE_SYS_SCREEN, 0, 0, 0);
    *cols = v & 0xFFu;
    *rows = (v >> 8) & 0xFFu;
}

/* Files: RAM only, 8 files of up to 512 bytes, one open handle per file, lost on reboot.
   Open with FLARE_FS_READ, FLARE_FS_WRITE (truncates) or FLARE_FS_APPEND. Each returns a
   handle (1-4) or FLARE_FS_FAIL. */
static inline flare_u32 flare_fs_open(const char *name, flare_u32 mode) {
    return flare_call(FLARE_SYS_FS_OPEN, (flare_u32)(unsigned long)name, mode, 0);
}

static inline flare_u32 flare_fs_read(flare_u32 h, void *buf, flare_u32 max) {
    return flare_call(FLARE_SYS_FS_READ, h, (flare_u32)(unsigned long)buf, max);
}

static inline flare_u32 flare_fs_write(flare_u32 h, const void *buf, flare_u32 count) {
    return flare_call(FLARE_SYS_FS_WRITE, h, (flare_u32)(unsigned long)buf, count);
}

static inline void flare_fs_close(flare_u32 h) {
    flare_call(FLARE_SYS_FS_CLOSE, h, 0, 0);
}

static inline flare_u32 flare_fs_delete(const char *name) {
    return flare_call(FLARE_SYS_FS_DELETE, (flare_u32)(unsigned long)name, 0, 0);
}

/* Copies the name of the file at index into name_out (13 bytes). Returns its size, or FLARE_FS_FAIL past the end. */
static inline flare_u32 flare_fs_list(flare_u32 index, char *name_out) {
    return flare_call(FLARE_SYS_FS_LIST, index, (flare_u32)(unsigned long)name_out, 0);
}

static inline flare_u32 flare_fs_size(flare_u32 h) {
    return flare_call(FLARE_SYS_FS_SIZE, h, 0, 0);
}

static inline flare_u32 flare_fs_seek(flare_u32 h, flare_u32 pos) {
    return flare_call(FLARE_SYS_FS_SEEK, h, pos, 0);
}

#define FLARE_APP() \
    void app_main(void); \
    static void flare_app_entry(void) { app_main(); flare_exit(); for (;;) { } } \
    __attribute__((section(".head"), used)) \
    const flare_u32 flare_app_head[2] = { FLARE_APP_MAGIC, (flare_u32)(unsigned long)flare_app_entry }

#endif
