#ifndef FS_H
#define FS_H

/* FlareOS RAM filesystem: a flat namespace of fixed-size files that live in kernel
   memory and vanish on reboot until a disk driver exists. A file can have one open
   handle at a time. Syscalls use int 0x80 like the rest of the ABI. */
#define SYS_FS_OPEN   21  /* ebx = name, ecx = FS_READ or FS_WRITE; returns handle (1-4) or FS_FAIL */
#define SYS_FS_READ   22  /* ebx = handle, ecx = buffer, edx = max bytes; returns bytes read or FS_FAIL */
#define SYS_FS_WRITE  23  /* ebx = handle, ecx = buffer, edx = byte count; returns bytes written or FS_FAIL */
#define SYS_FS_CLOSE  24  /* ebx = handle */
#define SYS_FS_DELETE 25  /* ebx = name; returns 0 or FS_FAIL */
#define SYS_FS_LIST   26  /* ebx = index among existing files, ecx = name buffer (FS_NAME_MAX + 1 bytes);
                             returns the file size or FS_FAIL past the last file */

#define SYS_FS_SIZE  27  /* ebx = handle; returns the file size in bytes */
#define SYS_FS_SEEK  28  /* ebx = handle, ecx = position (0..size); returns position or FS_FAIL */

#define FS_READ 0
#define FS_WRITE 1
#define FS_APPEND 2  /* write at the end, creating the file if needed */
#define FS_FAIL 0xFFFFFFFFu
#define FS_NAME_MAX 12
#define FS_FILE_MAX 512
#define FS_FILES 8
#define FS_HANDLES 4

static inline unsigned int fs_sys(unsigned int n, unsigned int b, unsigned int c, unsigned int d) {
    unsigned int ret;
    __asm__ volatile ("int $0x80" : "=a"(ret) : "a"(n), "b"(b), "c"(c), "d"(d) : "memory");
    return ret;
}

static inline unsigned int fs_open(const char *name, unsigned int mode) {
    return fs_sys(SYS_FS_OPEN, (unsigned int)(unsigned long)name, mode, 0);
}

static inline unsigned int fs_read(unsigned int h, void *buf, unsigned int max) {
    return fs_sys(SYS_FS_READ, h, (unsigned int)(unsigned long)buf, max);
}

static inline unsigned int fs_write(unsigned int h, const void *buf, unsigned int count) {
    return fs_sys(SYS_FS_WRITE, h, (unsigned int)(unsigned long)buf, count);
}

static inline void fs_close(unsigned int h) {
    fs_sys(SYS_FS_CLOSE, h, 0, 0);
}

static inline unsigned int fs_delete(const char *name) {
    return fs_sys(SYS_FS_DELETE, (unsigned int)(unsigned long)name, 0, 0);
}

static inline unsigned int fs_list(unsigned int index, char *name_out) {
    return fs_sys(SYS_FS_LIST, index, (unsigned int)(unsigned long)name_out, 0);
}

static inline unsigned int fs_size(unsigned int h) {
    return fs_sys(SYS_FS_SIZE, h, 0, 0);
}

static inline unsigned int fs_seek(unsigned int h, unsigned int pos) {
    return fs_sys(SYS_FS_SEEK, h, pos, 0);
}

#endif
