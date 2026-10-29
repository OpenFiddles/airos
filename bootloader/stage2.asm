; Stage 2 bootloader: sets up protected mode, A20, and jumps to kernel.
[BITS 16]
[ORG 0x9000]

stage2_start:
    mov [boot_drive], dl

    ; Enable A20 Line via Fast A20 method (System Control Port A)
    in al, 0x92
    or al, 2
    out 0x92, al

    ; Switch to 32-bit Protected Mode
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp dword 0x08:protected_entry

boot_drive db 0

gdt_start:
    dq 0
gdt_code:
    dw 0xFFFF, 0x0000
    db 0x00, 10011010b, 11001111b, 0x00
gdt_data:
    dw 0xFFFF, 0x0000
    db 0x00, 10010010b, 11001111b, 0x00
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

[BITS 32]
protected_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    
    ; Pass control to Textile Kernel Entry point at physical 0x10000
    mov eax, 0x10000
    jmp eax
