# FlareOS app SDK (v1.1)

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
| `flare_pointer()` | Pointer x, y (text cells) and buttons |
| `flare_screen(&cols, &rows)` | Text grid size (80 x 25 today) |
| `flare_exit()` | Return to the desktop (called automatically when `app_main` returns) |

### Files

| Function | What it does |
| --- | --- |
| `flare_fs_open(name, mode)` | Open with `FLARE_FS_READ`, `FLARE_FS_WRITE` (truncates) or `FLARE_FS_APPEND`. Returns a handle (1 to 4) or `FLARE_FS_FAIL` |
| `flare_fs_read(h, buf, max)` | Read up to `max` bytes; returns the count read |
| `flare_fs_write(h, buf, count)` | Write; returns the count written |
| `flare_fs_seek(h, pos)` | Move to `pos` (0 to size) |
| `flare_fs_size(h)` | Size of the open file |
| `flare_fs_close(h)` | Release the handle |
| `flare_fs_delete(name)` | Delete a file that has no open handle |
| `flare_fs_rename(old, new)` | Rename; fails if `new` exists or `old` is open (new in 1.1) |
| `flare_fs_exists(name)` | 1 if the file exists, else 0 (new in 1.1) |
| `flare_fs_list(index, name_out)` | Name and size of the file at `index`; `name_out` needs 13 bytes |
| `flare_fs_slots()` | Unused file slots out of 8 (new in 1.1) |

## Limits

- 4096 bytes per app.
- Names are up to 12 characters. A file holds up to 512 bytes. There are 8 files and 4 open handles.
- One open handle per file at a time.
- Files live in RAM and are lost on reboot. No disk driver exists yet.
- The disk image holds one app slot today, and `make sdk-example` does not put `hello.bin` on it. Launching apps from the desktop needs the app loader, which is not built yet.
- No window API or networking in the SDK yet.
- Syscall numbers must match `kernel/textile.h` and `include/fs.h`.

## Checks

`make fs-test` runs the filesystem checks on the host. The CI pipeline builds the kernel and the SDK example, but does not run the smoke test.
