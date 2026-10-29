#!/usr/bin/env python3
"""Print the header of an .airp file and check it against include/airp.h.

Usage: airpinfo.py FILE.airp
"""
import struct
import sys

AIRP_MAGIC = 0x50524941
AIRP_VERSION = 1
AIRP_API_LEVEL = 1
TX_APP_BASE = 0x20000
TX_APP_MAGIC = 0x54505041
HEADER = struct.Struct('<IHHIIII12s')


def main(argv):
    if len(argv) != 2:
        sys.exit(__doc__.strip())
    with open(argv[1], 'rb') as f:
        data = f.read()
    if len(data) < HEADER.size:
        sys.exit(f'airpinfo: file is {len(data)} bytes, shorter than the {HEADER.size}-byte header')
    magic, version, header_size, api, base, entry, size, raw_name = HEADER.unpack_from(data)
    name = raw_name.split(b'\0', 1)[0].decode('ascii', 'replace')
    problems = []
    if magic != AIRP_MAGIC:
        problems.append('bad magic')
    if version != AIRP_VERSION:
        problems.append(f'version {version}, expected {AIRP_VERSION}')
    if header_size != HEADER.size:
        problems.append(f'header size {header_size}, expected {HEADER.size}')
    if api > AIRP_API_LEVEL:
        problems.append(f'needs API level {api}, this kernel has {AIRP_API_LEVEL}')
    if base != TX_APP_BASE:
        problems.append(f'image base 0x{base:X}, expected 0x{TX_APP_BASE:X}')
    if not TX_APP_BASE <= entry < TX_APP_BASE + size:
        problems.append('entry point outside the image')
    if size > len(data) - HEADER.size:
        problems.append('image is cut short')
    image = data[HEADER.size:HEADER.size + size]
    if len(image) >= 8:
        head_magic, head_entry = struct.unpack_from('<II', image, 0)
        if head_magic != TX_APP_MAGIC or head_entry != entry:
            problems.append('app head does not match the header entry point')
    else:
        problems.append('image too small to hold an app head')
    print(f'name:     {name}')
    print(f'version:  {version}')
    print(f'api:      {api}')
    print(f'base:     0x{base:X}')
    print(f'entry:    0x{entry:X}')
    print(f'image:    {size} bytes')
    if problems:
        print('problems: ' + '; '.join(problems))
        sys.exit(1)
    print('ok')


if __name__ == '__main__':
    main(sys.argv)
