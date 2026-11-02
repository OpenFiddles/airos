#!/usr/bin/env python3
"""Wrap a linked FlareOS app image in an .airp container.

Usage: airpack.py IMAGE.bin OUT.airp NAME

IMAGE.bin is the flat binary from objcopy -O binary, linked with userland/app.ld.
The container layout is defined in include/airp.h.
"""
import struct
import sys

AIRP_MAGIC = 0x50524941
AIRP_VERSION = 1
AIRP_API_LEVEL = 1
TX_APP_BASE = 0x20000
TX_APP_MAGIC = 0x54505041
IMAGE_MAX = 4096
HEADER = struct.Struct('<IHHIIII12s')
assert HEADER.size == 36


def main(argv):
    if len(argv) != 4:
        sys.exit(__doc__.strip())
    src, dst, name = argv[1:]
    with open(src, 'rb') as f:
        image = f.read()
    if not 8 <= len(image) <= IMAGE_MAX:
        sys.exit(f'airpack: image is {len(image)} bytes; expected 8 to {IMAGE_MAX}')
    magic, entry = struct.unpack_from('<II', image, 0)
    if magic != TX_APP_MAGIC:
        sys.exit('airpack: image does not start with TX_APP_MAGIC; link it with userland/app.ld')
    if not TX_APP_BASE <= entry < TX_APP_BASE + len(image):
        sys.exit('airpack: entry address is outside the image')
    raw = name.encode('ascii')
    if not 1 <= len(raw) <= 11:
        sys.exit('airpack: name must be 1 to 11 ASCII characters')
    header = HEADER.pack(AIRP_MAGIC, AIRP_VERSION, HEADER.size, AIRP_API_LEVEL,
                         TX_APP_BASE, entry, len(image), raw.ljust(12, b'\0'))
    with open(dst, 'wb') as f:
        f.write(header + image)
    print(f'airpack: {name}: {HEADER.size + len(image)} bytes -> {dst}')


if __name__ == '__main__':
    main(sys.argv)
