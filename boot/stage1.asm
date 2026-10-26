; Stage 1 boot sector: loads the kernel (LBA 1..96) to 0x10000 and the app (LBA 97..104)
; to 0x20000, then enters 32-bit protected mode and jumps to the kernel entry.
[BITS 16]
[ORG 0x7C00]

KERNEL_SECTORS equ 96
APP_SECTORS equ 8

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drive], dl

    mov si, msg_loading
    call print16

    mov ax, 0x1000
    mov es, ax
    mov word [lba], 1
    mov word [offset], 0
    mov word [remaining], KERNEL_SECTORS
    call load_range

    mov ax, 0x2000
    mov es, ax
    mov word [offset], 0
    mov word [remaining], APP_SECTORS
    call load_range

    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp dword 0x08:protected_entry

load_range:
.load:
    cmp word [remaining], 0
    je .done
    mov ax, [lba]
    xor dx, dx
    mov cx, 36
    div cx
    mov [track], al
    mov ax, dx
    xor dx, dx
    mov cx, 18
    div cx
    mov dh, al
    mov cl, dl
    inc cl
    mov ch, [track]
    mov dl, [boot_drive]
    mov bx, [offset]
    mov ax, 0x0201
    int 0x13
    jc disk_error
    add word [offset], 512
    inc word [lba]
    dec word [remaining]
    jmp .load
.done:
    ret

disk_error:
    mov si, msg_disk
    call print16
.hang:
    hlt
    jmp .hang

print16:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    int 0x10
    jmp print16
.done:
    ret

[BITS 32]
protected_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    mov eax, 0x10000
    jmp eax

boot_drive  db 0
lba         dw 0
offset      dw 0
remaining   dw 0
track       db 0
msg_loading db "Loading AirOS...", 13, 10, 0
msg_disk    db "Disk read error", 13, 10, 0

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

times 510 - ($ - $$) db 0
dw 0xAA55
