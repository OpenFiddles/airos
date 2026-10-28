CC ?= gcc
AS = nasm
LD = ld
OBJCOPY = objcopy
QEMU ?= qemu-system-i386
BUILD = build

CFLAGS = -m32 -std=gnu99 -ffreestanding -fno-pic -fno-stack-protector -fno-builtin -fno-common -fno-tree-loop-distribute-patterns -fcf-protection=none -fno-asynchronous-unwind-tables -O2 -Wall -Wextra

# Expanded to compile all the core kernel files visible in your tree
OBJS = $(BUILD)/entry.o \
       $(BUILD)/isr.o \
       $(BUILD)/usermode.o \
       $(BUILD)/desktop.o \
       $(BUILD)/kernel.o \
       $(BUILD)/drivers.o \
       $(BUILD)/keyboard.o \
       $(BUILD)/panic.o \
       $(BUILD)/rtl8139.o \
       $(BUILD)/mouse_cursor.o \
       $(BUILD)/mouse_init.o \
       $(BUILD)/pit_timer.o

all: $(BUILD)/textile.img

$(BUILD):
	mkdir -p $(BUILD)

# --- Bootloader Stages ---
$(BUILD)/stage1.bin: boot/stage1.asm | $(BUILD)
	$(AS) -f bin $< -o $@

$(BUILD)/stage2.bin: boot/stage2.asm | $(BUILD)
	$(AS) -f bin $< -o $@
	truncate -s 2048 $@

# --- Kernel Core Assembly Objects ---
$(BUILD)/entry.o: kernel/entry.asm | $(BUILD)
	$(AS) -f elf32 $< -o $@

$(BUILD)/isr.o: kernel/isr.asm | $(BUILD)
	$(AS) -f elf32 $< -o $@

$(BUILD)/usermode.o: kernel/usermode.asm | $(BUILD)
	$(AS) -f elf32 $< -o $@

# --- Kernel Core C Objects ---
$(BUILD)/desktop.o: kernel/desktop.c kernel/kernel.h include/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/kernel.o: kernel/kernel.c kernel/kernel.h include/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/drivers.o: kernel/drivers.c kernel/kernel.h include/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/keyboard.o: kernel/keyboard.c kernel/kernel.h include/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/panic.o: kernel/panic.c kernel/kernel.h include/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/rtl8139.o: kernel/rtl8139.c kernel/kernel.h include/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Modular Subsystem Drivers ---
$(BUILD)/mouse_cursor.o: drivers/mouse_cursor.c kernel/kernel.h include/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/mouse_init.o: drivers/mouse_init.c kernel/kernel.h include/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/pit_timer.o: drivers/pit_timer.c kernel/kernel.h include/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Linking and Image Creation ---
$(BUILD)/kernel.elf: $(OBJS) kernel/link.ld
	$(LD) -m elf_i386 -nostdlib -T kernel/link.ld -o $@ $(OBJS)

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	test `stat -c %s $@` -le 49152
	truncate -s 49152 $@

# --- User Space App Objects ---
$(BUILD)/app.o: app/app.c include/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/rt.o: app/rt.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/app.elf: $(BUILD)/app.o $(BUILD)/rt.o app/app.ld
	$(LD) -m elf_i386 -nostdlib -T app/app.ld -o $@ $(BUILD)/app.o $(BUILD)/rt.o

$(BUILD)/app.bin: $(BUILD)/app.elf
	$(OBJCOPY) -O binary $< $@
	test `stat -c %s $@` -le 4096
	truncate -s 4096 $@

# Assembles the disk image using the precise structural sector positioning offsets
$(BUILD)/textile.img: $(BUILD)/stage1.bin $(BUILD)/stage2.bin $(BUILD)/kernel.bin $(BUILD)/app.bin
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	dd if=$(BUILD)/stage1.bin of=$@ bs=512 count=1 conv=notrunc status=none
	dd if=$(BUILD)/stage2.bin of=$@ bs=512 seek=1 count=4 conv=notrunc status=none
	dd if=$(BUILD)/kernel.bin of=$@ bs=512 seek=5 count=96 conv=notrunc status=none
	dd if=$(BUILD)/app.bin of=$@ bs=512 seek=101 count=8 conv=notrunc status=none

run: $(BUILD)/textile.img
	$(QEMU) -fda $(BUILD)/textile.img -serial stdio -no-reboot

test: $(BUILD)/textile.img
	python3 tests/smoke.py $(BUILD)/textile.img

clean:
	rm -rf $(BUILD)

.PHONY: all run test clean
