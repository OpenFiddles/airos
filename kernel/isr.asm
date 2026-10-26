[BITS 32]
section .text
global isr_timer
global isr_keyboard
global isr_fault
global isr_syscall
extern timer_irq
extern keyboard_irq
extern cpu_fault
extern syscall_dispatch

isr_timer:
    pushad
    call timer_irq
    popad
    iretd

isr_keyboard:
    pushad
    call keyboard_irq
    popad
    iretd

isr_fault:
    cli
    call cpu_fault
.halt:
    hlt
    jmp .halt

isr_syscall:
    pushad
    push esp
    call syscall_dispatch
    add esp, 4
    popad
    iretd
