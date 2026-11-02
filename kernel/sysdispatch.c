#include "kernel.h"

int axi_dispatch(struct saved_regs *r);
int fs_dispatch(struct saved_regs *r);
u32 syscall_dispatch(struct saved_regs *r);

/* isr.asm calls this for every syscall. Window and filesystem calls are tried first. */
unsigned int airos_dispatch(struct saved_regs *r) {
    if (!axi_dispatch(r) && !fs_dispatch(r))
        syscall_dispatch(r);
    return r->eax;
}
