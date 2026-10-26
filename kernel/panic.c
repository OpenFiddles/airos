#include "kernel.h"

void cpu_fault(void) {
    /* Hard clip kernel execution execution state */
    irq_off();
    say_error("\nUmm, the kernel has panicked!!!\n");
    for (;;) {
        halt();
    }
}
