CC ?= gcc
AS = nasm
LD = ld
OBJCOPY = objcopy
QEMU ?= qemu-system-i386
BUILD = build
CFLAGS = -m32 -std=gnu99 -ffreestanding -fno-pic -fno-stack-protector -fno-builtin -fno-common -fno-tree-loop-distribute-patterns -fcf-protection=none -fno-asynchronous-unwind-tables -O2 -Wall -Wextra
OBJS = $(BUILD)/entry.o $(BUILD)/isr.o $(BUILD)/kernel.o

all: $(BUILD)/textile.img

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/stage1.bin: boot/stage1.asm | $(BUILD)
	$(AS) -f bin $< -o $@

$(BUILD)/entry.o: kernel/entry.asm | $(BUILD)
	$(AS) -f elf32 $< -o $@

$(BUILD)/isr.o: kernel/isr.asm | $(BUILD)
	$(AS) -f elf32 $< -o $@

$(BUILD)/kernel.o: kernel/kernel.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/kernel.elf: $(OBJS) kernel/link.ld
	$(LD) -m elf_i386 -nostdlib -T kernel/link.ld -o $@ $(OBJS)

$(BUILD)/kernel.bin: $(BUILD)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	test `stat -c %s $@` -le 32768
	truncate -s 32768 $@

$(BUILD)/textile.img: $(BUILD)/stage1.bin $(BUILD)/kernel.bin
	cat $(BUILD)/stage1.bin $(BUILD)/kernel.bin > $@
	truncate -s 1474560 $@

run: $(BUILD)/textile.img
	$(QEMU) -fda $(BUILD)/textile.img -serial stdio -no-reboot

test: $(BUILD)/textile.img
	python3 tests/smoke.py $(BUILD)/textile.img

clean:
	rm -rf $(BUILD)

.PHONY: all run test clean
