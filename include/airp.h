#ifndef AIRP_H
#define AIRP_H

/* .airp container, version 1. Little-endian: a 36-byte header, then the flat app image.
   The image is what the kernel loads at TX_APP_BASE: its first two words are TX_APP_MAGIC
   and the absolute address of the entry function (kernel/textile.h). tools/airpack.py
   writes this format. The kernel loader that reads .airp files is not written yet. */

#define AIRP_MAGIC        0x50524941u  /* bytes on disk: "AIRP" */
#define AIRP_VERSION      1
#define AIRP_API_LEVEL    1            /* window API 14-20, filesystem API 21-26 */
#define AIRP_NAME_MAX     12           /* name is NUL padded, at most 11 characters */
#define AIRP_IMAGE_MAX    4096         /* one app slot on disk (8 sectors) */

struct airp_header {
    unsigned int magic;
    unsigned short version;
    unsigned short header_size;
    unsigned int api_level;
    unsigned int image_base;   /* must equal TX_APP_BASE in version 1 */
    unsigned int entry;        /* absolute address inside the image */
    unsigned int image_size;   /* bytes after the header */
    char name[AIRP_NAME_MAX];
} __attribute__((packed));

#endif
