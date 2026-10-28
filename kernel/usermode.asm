[BITS 32]
section .text
global gdt_flush
global tss_flush
global enter_user
global user_leave

gdt_flush:
    mov eax, [esp + 4]
    lgdt [eax]
    jmp 0x08:reload_segments
reload_segments:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

tss_flush:
    mov ax, 0x28
    ltr ax
    ret

enter_user:
    mov eax, [esp + 4]
    mov ecx, [esp + 8]
    push ebp
    push ebx
    push esi
    push edi
    mov [kernel_resume_esp], esp
    push dword 0x23
    push ecx
    push dword 0x202
    push dword 0x1B
    push eax
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    iretd

user_leave:
    mov esp, [kernel_resume_esp]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret

section .data
kernel_resume_esp: dd 0
