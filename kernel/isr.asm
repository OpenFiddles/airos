[BITS 32]
section .text
global isr_timer
global isr_keyboard
global isr_fault
global isr_syscall
global isr_ne2k

extern timer_irq
extern keyboard_irq
extern cpu_fault
extern airos_dispatch
extern ne2k_irq

isr_timer:
    pushad          ; Push EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    
    ; Mandatory for C calling convention (cdecl)
    cld             ; Clear direction flag (ensures string functions move forward)
    
    call timer_irq
    
    ; Send End-of-Interrupt (EOI) to the PIC
    mov al, 0x20
    out 0x20, al    ; Master PIC Command Port
    
    popad
    iretd

isr_keyboard:
    pushad
    cld
    
    call keyboard_irq
    
    ; Send End-of-Interrupt (EOI) to the PIC
    mov al, 0x20
    out 0x20, al    ; Master PIC Command Port
    
    popad
    iretd

isr_fault:
    cli             ; Disable interrupts permanently for panic state
    
    ; Faults push an error code onto the stack for *some* exceptions (like Page Faults).
    ; For a simple baseline, we preserve the stack and push the current ESP as an argument.
    push esp        
    call cpu_fault  ; void cpu_fault(uint32_t *registers);
    
    ; If cpu_fault returns, securely halt
.halt:
    hlt
    jmp .halt

isr_syscall:
    pushad
    cld
    
    ; Pass the pointer to all saved registers to C; airos_dispatch tries the window
    ; and filesystem handlers first, then the base syscall table.
    push esp        
    call airos_dispatch
    add esp, 4
    
    ; EAX is the standard return register in C/x86. 
    ; We overwrite the saved EAX on the stack so popad restores the return value.
    mov [esp + 28], eax 
    
    popad
    iretd

isr_ne2k:
    pushad
    cld
    call ne2k_irq
    mov al, 0x20
    out 0x20, al
    popad
    iretd

global isr_mouse
extern mouse_irq_handler

isr_mouse:
    pushad
    cld
    call mouse_irq_handler
    mov al, 0x20
    out 0xA0, al
    out 0x20, al
    popad
    iretd
