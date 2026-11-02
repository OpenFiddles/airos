#include "kernel.h"

int axi_dispatch(struct saved_regs *r);
int fs_dispatch(struct saved_regs *r);
u32 syscall_dispatch(struct saved_regs *r);

/* Routes a syscall through the window and filesystem handlers first.
   isr.asm currently calls syscall_dispatch directly, so this is not used yet. */
unsigned int airos_route(struct saved_regs *r) {
    if (!axi_dispatch(r) && !fs_dispatch(r))
        syscall_dispatch(r);
    return r->eax;
}
