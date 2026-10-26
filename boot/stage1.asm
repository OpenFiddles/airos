; Stage 1 boot sector: loads Stage 2 (LBA 1..4) to 0x0000:0x9000,
; the kernel (LBA 5..100) to 0x10000, and the app (LBA 101..108) to 0x20000.
[BITS 16]
[ORG 0x7C00]

STAGE2_SECTORS equ 4
KERNEL_SECTORS equ 96
APP_SECTORS    equ 8

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

    ; Load Stage 2 to 0x0000:0x9000
    xor ax, ax
    mov es, ax
    mov word [lba], 1
    mov word [offset], 0x9000
    mov word [remaining], STAGE2_SECTORS
    call load_range

    ; Load Kernel to 0x1000:0x0000 (Physical 0x10000)
    mov ax, 0x1000
    mov es, ax
    mov word [offset], 0
    mov word [remaining], KERNEL_SECTORS
    call load_range

    ; Load App to 0x2000:0x0000 (Physical 0x20000)
    mov ax, 0x2000
    mov es, ax
    mov word [offset], 0
    mov word [remaining], APP_SECTORS
    call load_range

    ; Pass boot drive to Stage 2 in DL and jump
    mov dl, [boot_drive]
    jmp 0x0000:0x9000

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
    
    ; Handle segment overflow checks safely
    add word [offset], 512
    jnz .no_carry
    mov ax, es
    add ax, 0x1000
    mov es, ax
.no_carry:
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

boot_drive  db 0
lba         dw 0
offset      dw 0
remaining   dw 0
track       db 0
msg_loading db "Loading AirOS...", 13, 10, 0
msg_disk    db "Disk read error", 13, 10, 0

times 510 - ($ - $$) db 0
dw 0xAA55
