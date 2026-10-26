[BITS 32]
section .text.entry
global _start
extern kmain
extern __bss_start
extern __bss_end

_start:
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    cld
    rep stosb
    call kmain
.halt:
    cli
    hlt
    jmp .halt
