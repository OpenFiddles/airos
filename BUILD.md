# Building FlareOS

A small x86 hobby kernel: a 512-byte boot sector loads a 64-sector kernel, switches to 32-bit protected mode, and runs a shell.

- Hardware it talks to: VGA text (0xB8000 and cursor ports), PS/2 keyboard (IRQ1), PIT timer (IRQ0), PIC, CMOS clock (ports 0x70/0x71), RTL8139 network card (PCI config and I/O ports, detection only for now), COM1 serial (mirrors the screen for tests), and any I/O port through `in` and `out`.
- Shell commands: `help`, `clear`, `echo`, `time`, `uptime`, `in PORT`, `out PORT VALUE`, `reboot`, `net`, `sqr X Y`.
- Boot shows a short Saturn splash with a pulsing background before the shell starts.

## Build and run

    make            # builds build/textile.img (1.44 MB floppy image)
    make run        # boots in QEMU, serial output in the terminal
    make test       # headless boot + keyboard test (tests/smoke.py), with an RTL8139 attached

Needs nasm, gcc with 32-bit support (gcc-multilib on Debian/Ubuntu), ld, objcopy and qemu-system-i386.
