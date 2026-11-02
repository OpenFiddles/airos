CC ?= gcc
AS = nasm
LD = ld
OBJCOPY = objcopy
QEMU ?= qemu-system-i386
BUILD = build

DRV_SRCS = $(wildcard drivers/*.c)
DRV_OBJS = $(patsubst drivers/%.c,$(BUILD)/d_%.o,$(DRV_SRCS))

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
       $(BUILD)/mouse_cursor.o \
       $(BUILD)/mouse_init.o \
       $(BUILD)/pit_timer.o \
       $(DRV_OBJS) \
       $(BUILD)/axi.o \
       $(BUILD)/fs.o \
       $(BUILD)/sysdispatch.o \
       $(BUILD)/airp.o

all: $(BUILD)/textile.img

$(BUILD):
	mkdir -p $(BUILD)

# --- Bootloader Stages ---
$(BUILD)/stage1.bin: bootloader/stage1.asm | $(BUILD)
	$(AS) -f bin $< -o $@

$(BUILD)/stage2.bin: bootloader/stage2.asm | $(BUILD)
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
$(BUILD)/desktop.o: kernel/desktop.c kernel/kernel.h kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/kernel.o: kernel/kernel.c kernel/kernel.h kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/drivers.o: kernel/drivers.c kernel/kernel.h kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/keyboard.o: kernel/keyboard.c kernel/kernel.h kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/panic.o: kernel/panic.c kernel/kernel.h kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/rtl8139.o: kernel/rtl8139.c kernel/kernel.h kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/axi.o: kernel/axi.c include/axi.h kernel/kernel.h kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/fs.o: kernel/fs.c include/fs.h kernel/kernel.h kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/sysdispatch.o: kernel/sysdispatch.c kernel/kernel.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/airp.o: kernel/airp.c include/airp.h kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

# --- Modular Subsystem Drivers ---
$(BUILD)/mouse_cursor.o: kernel/mouse_cursor.c kernel/kernel.h kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/mouse_init.o: kernel/mouse_init.c kernel/kernel.h kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/pit_timer.o: kernel/pit_timer.c kernel/kernel.h kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/drv_pic.o: drivers/drv_pic.c include/drv_pic.h include/hw.h | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD)/d_%.o: drivers/%.c kernel/kernel.h kernel/textile.h include/*.h | $(BUILD)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

# --- Linking and Image Creation ---
$(BUILD)/kernel.elf: $(OBJS) kernel/link.ld
	$(LD) -m elf_i386 -nostdlib -T kernel/link.ld -o $@ $(OBJS)

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	test `stat -c %s $@` -le 49152
	truncate -s 49152 $@

# --- User Space App Objects ---
$(BUILD)/app.o: userland/app.c kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/rt.o: userland/rt.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/app.elf: $(BUILD)/app.o $(BUILD)/rt.o userland/app.ld
	$(LD) -m elf_i386 -nostdlib -T userland/app.ld -o $@ $(BUILD)/app.o $(BUILD)/rt.o

$(BUILD)/app.bin: $(BUILD)/app.elf
	$(OBJCOPY) -O binary $< $@
	test `stat -c %s $@` -le 4096
	truncate -s 4096 $@

# --- Explorer as .airp (stage 1 loads it from LBA 109 to 0x30000) ---
$(BUILD)/explorer.o: userland/explorer.c include/axi.h include/fs.h kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/explorer.elf: $(BUILD)/explorer.o $(BUILD)/rt.o userland/app.ld
	$(LD) -m elf_i386 -nostdlib -T userland/app.ld -o $@ $(BUILD)/explorer.o $(BUILD)/rt.o

$(BUILD)/explorer.bin: $(BUILD)/explorer.elf
	$(OBJCOPY) -O binary $< $@
	test `stat -c %s $@` -le 4096

$(BUILD)/explorer.airp: $(BUILD)/explorer.bin tools/airpack.py
	python3 tools/airpack.py $(BUILD)/explorer.bin $@ explorer

# Assembles the disk image using the precise structural sector positioning offsets
$(BUILD)/textile.img: $(BUILD)/stage1.bin $(BUILD)/stage2.bin $(BUILD)/kernel.bin $(BUILD)/app.bin $(BUILD)/explorer.airp
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	dd if=$(BUILD)/stage1.bin of=$@ bs=512 count=1 conv=notrunc status=none
	dd if=$(BUILD)/stage2.bin of=$@ bs=512 seek=1 count=4 conv=notrunc status=none
	dd if=$(BUILD)/kernel.bin of=$@ bs=512 seek=5 count=96 conv=notrunc status=none
	dd if=$(BUILD)/app.bin of=$@ bs=512 seek=101 count=8 conv=notrunc status=none
	dd if=$(BUILD)/explorer.airp of=$@ bs=512 seek=109 count=9 conv=notrunc status=none

# --- Window API demo: same image layout, app slot holds userland/win_app.c ---
$(BUILD)/win_app.o: userland/win_app.c include/axi.h kernel/textile.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/win_app.elf: $(BUILD)/win_app.o $(BUILD)/rt.o userland/app.ld
	$(LD) -m elf_i386 -nostdlib -T userland/app.ld -o $@ $(BUILD)/win_app.o $(BUILD)/rt.o

$(BUILD)/win_app.bin: $(BUILD)/win_app.elf
	$(OBJCOPY) -O binary $< $@
	test `stat -c %s $@` -le 4096
	test `stat -c %s $@` -le 4096

$(BUILD)/win-demo.img: $(BUILD)/stage1.bin $(BUILD)/stage2.bin $(BUILD)/kernel.bin $(BUILD)/win_app.bin
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	dd if=$(BUILD)/stage1.bin of=$@ bs=512 count=1 conv=notrunc status=none
	dd if=$(BUILD)/stage2.bin of=$@ bs=512 seek=1 count=4 conv=notrunc status=none
	dd if=$(BUILD)/kernel.bin of=$@ bs=512 seek=5 count=96 conv=notrunc status=none
	dd if=$(BUILD)/win_app.bin of=$@ bs=512 seek=101 count=8 conv=notrunc status=none

run: $(BUILD)/textile.img
	$(QEMU) -fda $(BUILD)/textile.img -serial stdio -no-reboot

win-demo: $(BUILD)/win-demo.img
	$(QEMU) -fda $(BUILD)/win-demo.img -serial stdio -no-reboot

test: $(BUILD)/textile.img
	python3 tests/smoke.py $(BUILD)/textile.img

clean:
	rm -rf $(BUILD)

.PHONY: all run win-demo test clean

# --- SDK example app (built on request, not on the disk image) ---
$(BUILD)/hello.o: examples/hello/hello.c sdk/flareos.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/hello.elf: $(BUILD)/hello.o $(BUILD)/rt.o userland/app.ld
	$(LD) -m elf_i386 -nostdlib -T userland/app.ld -o $@ $(BUILD)/hello.o $(BUILD)/rt.o

$(BUILD)/hello.bin: $(BUILD)/hello.elf
	$(OBJCOPY) -O binary $< $@
	test `stat -c %s $@` -le 4096

sdk-example: $(BUILD)/hello.bin

.PHONY: sdk-example

# --- Host test for the RAM filesystem (runs on this machine, not in the kernel) ---
fs-test: | $(BUILD)
	$(CC) -m32 -std=gnu99 -Wall -Wextra -Iinclude -o $(BUILD)/fs_host_test tests/fs_host_test.c
	$(BUILD)/fs_host_test

.PHONY: fs-test
