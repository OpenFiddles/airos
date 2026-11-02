# FlareOS app SDK (v1)

Apps are flat x86 binaries. They run in ring 3 and call the kernel with `int 0x80`. The header is `sdk/flareos.h`.

## Make an app

1. Include the header: `#include "flareos.h"`.
2. Write `void app_main(void)`.
3. Put `FLARE_APP();` once at file level. It places the header the kernel reads.

`examples/hello/hello.c` is a working example. `make sdk-example` builds it to `build/hello.bin` and fails if it is over 4096 bytes.

## API

| Function | What it does |
| --- | --- |
| `flare_print(s)` | Print a string to the text screen |
| `flare_putc(ch)` | Print one character at the cursor |
| `flare_put_cell(row, col, ch, attr)` | Draw one cell (80x25 VGA text) |
| `flare_clear()` | Clear the screen |
| `flare_ticks()` | Timer ticks since boot (about 100 per second) |
| `flare_sleep(ticks)` | Wait for ticks |
| `flare_tone(hz, ms)` | Play a tone (20 to 20000 Hz; 0 turns the speaker off) |
| `flare_anim_start(slot, ease, ticks, from, to)` | Start a tween in one of 16 slots |
| `flare_anim_value(slot)` | Read a tween's current value |
| `flare_getkey()` | Next keyboard character, or -1 if none |
| `flare_get_arrow()` | Arrow code, or -1. The kernel does not dispatch this yet, so it returns -1 today |
| `flare_exit()` | Return to the desktop (called automatically when `app_main` returns) |

## Limits

- 4096 bytes per app.
- The disk image holds one app slot today, and `make sdk-example` does not put `hello.bin` on it. Launching apps from the desktop needs the app loader, which is not built yet.
- No window API, files, or networking yet.
- Syscall numbers must match `kernel/textile.h`.

## Pointer, screen and files (v1, added with the phone build)

- `flare_pointer()` returns `x`, `y` in text cells and `buttons` (bit 0 = left). The pointer is the PS/2 mouse; on a phone, touch drags move it and taps click.
- `flare_screen(&cols, &rows)` returns the text grid size. The kernel is 80x25 VGA text, so this returns 80 and 25 today. The browser page scales the text to the phone's width.
- Files: `flare_fs_open(name, mode)` with `FLARE_FS_READ`, `FLARE_FS_WRITE` (truncates) or `FLARE_FS_APPEND`; `flare_fs_read`, `flare_fs_write`, `flare_fs_seek`, `flare_fs_size`, `flare_fs_close`, `flare_fs_delete`, `flare_fs_list`. Names are up to 12 characters, a file is up to 512 bytes, there are 8 files and 4 open handles. Files live in RAM and are lost on reboot.

`make fs-test` runs the filesystem checks on the host.
