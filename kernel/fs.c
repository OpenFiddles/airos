/*
FILE PATH: kernel/fs.c
CONTRIBUTOR (Optional): Textile
PURPOSE: RAM-backed flat filesystem for FlareOS: 8 files of up to 512 bytes, 4 open handles, and the dispatcher for syscalls 21-33. Contents are lost on reboot.
*/

#include "kernel.h"
#include "../include/fs.h"

struct fs_file {
    u8 used;
    char name[FS_NAME_MAX + 1];
    u32 size;
    u8 data[FS_FILE_MAX];
};

struct fs_handle {
    u8 used;
    u8 mode;
    u8 file;
    u32 pos;
};

static struct fs_file fsk_files[FS_FILES];
static struct fs_handle fsk_handles[FS_HANDLES];

static int fsk_copy_name(char *dst, u32 src) {
    const char *s = (const char *)src;
    int n = 0;
    if (!s)
        return 0;
    while (s[n]) {
        if (n >= FS_NAME_MAX)
            return 0;
        dst[n] = s[n];
        n++;
    }
    if (n == 0)
        return 0;
    dst[n] = 0;
    return 1;
}

static int fsk_name_eq(const char *a, const char *b) {
    int i = 0;
    while (a[i] && a[i] == b[i])
        i++;
    return a[i] == b[i];
}

static int fsk_find(const char *name) {
    for (int i = 0; i < FS_FILES; i++)
        if (fsk_files[i].used && fsk_name_eq(fsk_files[i].name, name))
            return i;
    return -1;
}

static int fsk_file_busy(int file) {
    for (int i = 0; i < FS_HANDLES; i++)
        if (fsk_handles[i].used && fsk_handles[i].file == file)
            return 1;
    return 0;
}

static int fsk_free_file(void) {
    for (int i = 0; i < FS_FILES; i++)
        if (!fsk_files[i].used)
            return i;
    return -1;
}

static int fsk_free_handle(void) {
    for (int i = 0; i < FS_HANDLES; i++)
        if (!fsk_handles[i].used)
            return i;
    return -1;
}

static struct fs_handle *fsk_get(u32 h) {
    if (h < 1 || h > FS_HANDLES)
        return 0;
    struct fs_handle *s = &fsk_handles[h - 1];
    return s->used ? s : 0;
}

static u32 fsk_open(u32 name_ptr, u32 mode) {
    char name[FS_NAME_MAX + 1];
    int file, slot, i;

    if (mode > FS_APPEND || !fsk_copy_name(name, name_ptr))
        return FS_FAIL;
    file = fsk_find(name);
    if (file >= 0 && fsk_file_busy(file))
        return FS_FAIL;
    if (file < 0 && mode == FS_READ)
        return FS_FAIL;
    slot = fsk_free_handle();
    if (slot < 0)
        return FS_FAIL;
    if (file < 0) {
        file = fsk_free_file();
        if (file < 0)
            return FS_FAIL;
        fsk_files[file].used = 1;
        fsk_files[file].size = 0;
        i = 0;
        while (name[i]) {
            fsk_files[file].name[i] = name[i];
            i++;
        }
        fsk_files[file].name[i] = 0;
    }
    if (mode == FS_WRITE)
        fsk_files[file].size = 0;
    fsk_handles[slot].used = 1;
    fsk_handles[slot].mode = (u8)mode;
    fsk_handles[slot].file = (u8)file;
    fsk_handles[slot].pos = mode == FS_APPEND ? fsk_files[file].size : 0;
    return (u32)(slot + 1);
}

static u32 fsk_read(u32 h, u32 buf, u32 max) {
    struct fs_handle *s = fsk_get(h);
    struct fs_file *f;
    u32 left, n;
    u8 *dst = (u8 *)buf;

    if (!s || s->mode != FS_READ || !buf)
        return FS_FAIL;
    f = &fsk_files[s->file];
    left = f->size > s->pos ? f->size - s->pos : 0;
    n = max < left ? max : left;
    for (u32 i = 0; i < n; i++)
        dst[i] = f->data[s->pos + i];
    s->pos += n;
    return n;
}

static u32 fsk_write(u32 h, u32 buf, u32 count) {
    struct fs_handle *s = fsk_get(h);
    struct fs_file *f;
    u32 room, n;
    const u8 *src = (const u8 *)buf;

    if (!s || s->mode == FS_READ || !buf)
        return FS_FAIL;
    f = &fsk_files[s->file];
    room = FS_FILE_MAX - s->pos;
    n = count < room ? count : room;
    for (u32 i = 0; i < n; i++)
        f->data[s->pos + i] = src[i];
    s->pos += n;
    if (s->pos > f->size)
        f->size = s->pos;
    return n;
}

static u32 fsk_close(u32 h) {
    struct fs_handle *s = fsk_get(h);
    if (!s)
        return FS_FAIL;
    s->used = 0;
    return 0;
}

static u32 fsk_delete(u32 name_ptr) {
    char name[FS_NAME_MAX + 1];
    int file;

    if (!fsk_copy_name(name, name_ptr))
        return FS_FAIL;
    file = fsk_find(name);
    if (file < 0 || fsk_file_busy(file))
        return FS_FAIL;
    fsk_files[file].used = 0;
    fsk_files[file].size = 0;
    return 0;
}

static u32 fsk_list(u32 index, u32 name_out) {
    char *dst = (char *)name_out;
    int k;

    if (!name_out)
        return FS_FAIL;
    for (int i = 0; i < FS_FILES; i++) {
        if (!fsk_files[i].used)
            continue;
        if (index == 0) {
            for (k = 0; fsk_files[i].name[k]; k++)
                dst[k] = fsk_files[i].name[k];
            dst[k] = 0;
            return fsk_files[i].size;
        }
        index--;
    }
    return FS_FAIL;
}

static u32 fsk_size(u32 h) {
    struct fs_handle *s = fsk_get(h);
    if (!s)
        return FS_FAIL;
    return fsk_files[s->file].size;
}

static u32 fsk_seek(u32 h, u32 pos) {
    struct fs_handle *s = fsk_get(h);
    if (!s || pos > fsk_files[s->file].size)
        return FS_FAIL;
    s->pos = pos;
    return pos;
}

static u32 fsk_rename(u32 old_ptr, u32 new_ptr) {
    char old_name[FS_NAME_MAX + 1];
    char new_name[FS_NAME_MAX + 1];
    int file, i;

    if (!fsk_copy_name(old_name, old_ptr) || !fsk_copy_name(new_name, new_ptr))
        return FS_FAIL;
    file = fsk_find(old_name);
    if (file < 0 || fsk_file_busy(file) || fsk_find(new_name) >= 0)
        return FS_FAIL;
    for (i = 0; new_name[i]; i++)
        fsk_files[file].name[i] = new_name[i];
    fsk_files[file].name[i] = 0;
    return 0;
}

static u32 fsk_exists(u32 name_ptr) {
    char name[FS_NAME_MAX + 1];

    if (!fsk_copy_name(name, name_ptr))
        return 0;
    return fsk_find(name) >= 0 ? 1 : 0;
}

static u32 fsk_slots(void) {
    u32 n = 0;
    for (int i = 0; i < FS_FILES; i++)
        if (!fsk_files[i].used)
            n++;
    return n;
}

int fs_dispatch(struct saved_regs *r) {
    switch (r->eax) {
    case SYS_FS_OPEN:
        r->eax = fsk_open(r->ebx, r->ecx);
        return 1;
    case SYS_FS_READ:
        r->eax = fsk_read(r->ebx, r->ecx, r->edx);
        return 1;
    case SYS_FS_WRITE:
        r->eax = fsk_write(r->ebx, r->ecx, r->edx);
        return 1;
    case SYS_FS_CLOSE:
        r->eax = fsk_close(r->ebx);
        return 1;
    case SYS_FS_DELETE:
        r->eax = fsk_delete(r->ebx);
        return 1;
    case SYS_FS_LIST:
        r->eax = fsk_list(r->ebx, r->ecx);
        return 1;
    case SYS_FS_SIZE:
        r->eax = fsk_size(r->ebx);
        return 1;
    case SYS_FS_SEEK:
        r->eax = fsk_seek(r->ebx, r->ecx);
        return 1;
    case SYS_FS_RENAME:
        r->eax = fsk_rename(r->ebx, r->ecx);
        return 1;
    case SYS_FS_EXISTS:
        r->eax = fsk_exists(r->ebx);
        return 1;
    case SYS_FS_SLOTS:
        r->eax = fsk_slots();
        return 1;
    default:
        return 0;
    }
}
