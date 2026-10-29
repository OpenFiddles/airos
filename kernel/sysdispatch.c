#include "kernel.h"

int axi_dispatch(struct saved_regs *r);
int fs_dispatch(struct saved_regs *r);
void syscall_dispatch(struct saved_regs *r);

unsigned int airos_dispatch(struct saved_regs *r) {
    if (!axi_dispatch(r) && !fs_dispatch(r))
        syscall_dispatch(r);
    return r->eax;
}
