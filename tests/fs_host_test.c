#include <stdio.h>
#include <string.h>
#define putc fsk_test_putc
#define puts fsk_test_puts
#include "../kernel/fs.c"

static int failures;

static void check(int ok, const char *what) {
    printf("%s %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        failures++;
}

static u32 sys(u32 n, u32 b, u32 c, u32 d) {
    struct saved_regs r;
    memset(&r, 0, sizeof r);
    r.eax = n;
    r.ebx = b;
    r.ecx = c;
    r.edx = d;
    if (!fs_dispatch(&r)) {
        printf("FAIL no handler for %u\n", n);
        failures++;
        return FS_FAIL;
    }
    return r.eax;
}

static u32 addr(const void *p) {
    return (u32)(unsigned long)p;
}

int main(void) {
    char buf[32], big[600], name[8];
    u32 h, n, ok;

    h = sys(SYS_FS_OPEN, addr("notes"), FS_WRITE, 0);
    check(h != FS_FAIL, "open notes for write");
    check(sys(SYS_FS_WRITE, h, addr("hello"), 5) == 5, "write hello");
    sys(SYS_FS_CLOSE, h, 0, 0);

    h = sys(SYS_FS_OPEN, addr("notes"), FS_APPEND, 0);
    check(h != FS_FAIL, "open notes for append");
    check(sys(SYS_FS_WRITE, h, addr(" world"), 6) == 6, "append world");
    sys(SYS_FS_CLOSE, h, 0, 0);

    h = sys(SYS_FS_OPEN, addr("notes"), FS_READ, 0);
    check(sys(SYS_FS_SIZE, h, 0, 0) == 11, "size is 11");
    memset(buf, 0, sizeof buf);
    n = sys(SYS_FS_READ, h, addr(buf), 31);
    check(n == 11 && strcmp(buf, "hello world") == 0, "read back hello world");
    check(sys(SYS_FS_SEEK, h, 6, 0) == 6, "seek to 6");
    memset(buf, 0, sizeof buf);
    n = sys(SYS_FS_READ, h, addr(buf), 5);
    check(n == 5 && strcmp(buf, "world") == 0, "read world after seek");
    check(sys(SYS_FS_SEEK, h, 12, 0) == FS_FAIL, "seek past end fails");
    check(sys(SYS_FS_OPEN, addr("notes"), FS_READ, 0) == FS_FAIL, "second open of a busy file fails");
    sys(SYS_FS_CLOSE, h, 0, 0);

    h = sys(SYS_FS_OPEN, addr("notes"), FS_WRITE, 0);
    check(h != FS_FAIL && sys(SYS_FS_SIZE, h, 0, 0) == 0, "write mode truncates");
    sys(SYS_FS_CLOSE, h, 0, 0);

    memset(big, 'x', sizeof big);
    h = sys(SYS_FS_OPEN, addr("big"), FS_WRITE, 0);
    check(sys(SYS_FS_WRITE, h, addr(big), 600) == 512, "write stops at 512 bytes");
    sys(SYS_FS_CLOSE, h, 0, 0);
    check(sys(SYS_FS_DELETE, addr("big"), 0, 0) == 0, "delete big");

    check(sys(SYS_FS_DELETE, addr("notes"), 0, 0) == 0, "delete notes");
    check(sys(SYS_FS_OPEN, addr("notes"), FS_READ, 0) == FS_FAIL, "notes is gone");
    check(sys(SYS_FS_OPEN, addr("waytoolongname123"), FS_WRITE, 0) == FS_FAIL, "over-long name rejected");

    ok = 1;
    for (int i = 0; i < FS_FILES; i++) {
        name[0] = 'f';
        name[1] = (char)('0' + i);
        name[2] = 0;
        h = sys(SYS_FS_OPEN, addr(name), FS_WRITE, 0);
        if (h == FS_FAIL)
            ok = 0;
        sys(SYS_FS_CLOSE, h, 0, 0);
    }
    check(ok, "eight files fit");
    check(sys(SYS_FS_OPEN, addr("extra"), FS_WRITE, 0) == FS_FAIL, "ninth file is refused");
    check(sys(SYS_FS_LIST, 0, addr(buf), 0) == 0 && strcmp(buf, "f0") == 0, "list index 0 is f0");
    check(sys(SYS_FS_LIST, FS_FILES, addr(buf), 0) == FS_FAIL, "list past the last file fails");

    check(sys(SYS_FS_DELETE, addr("f0"), 0, 0) == 0, "delete f0");
    h = sys(SYS_FS_OPEN, addr("newfile"), FS_APPEND, 0);
    check(h != FS_FAIL, "append creates a new file");
    sys(SYS_FS_CLOSE, h, 0, 0);

    printf("%s: %d failure(s)\n", failures ? "FAILED" : "OK", failures);
    return failures ? 1 : 0;
}
