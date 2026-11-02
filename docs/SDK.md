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
