#include "../kernel/kernel.h"

static int ps2_mouse_init(void);

static struct driver mouse_hardware_driver = {
    .name = "PS/2 Intel Mouse Core",
    .type = DRV_TYPE_UNKNOWN, 
    .init = ps2_mouse_init,
    .shutdown = 0
};

/* Core helper functions to talk to the keyboard/mouse controller ports */
static inline void mouse_wait_write(void) {
    while ((inb(0x64) & 2) != 0); // Wait until input buffer is clear
}

static inline void mouse_wait_read(void) {
    while ((inb(0x64) & 1) == 0); // Wait until data is ready to be read
}

static inline void mouse_write_command(u8 command) {
    mouse_wait_write();
    outb(0x64, 0xD4); // Tell controller to route next byte straight to auxiliary mouse device
    mouse_wait_write();
    outb(0x60, command);
}

static inline u8 mouse_read_data(void) {
    mouse_wait_read();
    return inb(0x60);
}

/* Kicks off physical configuration sequencing */
static int ps2_mouse_init(void) {
    u8 status;

    // 1. Enable Auxiliary Mouse Device on controller
    mouse_wait_write();
    outb(0x64, 0xA8);

    // 2. Enable Mouse Interrupts (IRQ 12) by editing the controller configuration byte
    mouse_wait_write();
    outb(0x64, 0x20); // Command: Read Compack Status Byte
    mouse_wait_read();
    status = (inb(0x60) | 2); // Set bit 1 to unmask IRQ 12
    mouse_wait_write();
    outb(0x64, 0x60); // Command: Write Compack Status Byte
    mouse_wait_write();
    outb(0x60, status);

    // 3. Set default resolution and sampling scales on raw mouse firmware
    mouse_write_command(0xF6); // Set default device defaults
    mouse_read_data();         // Read ACK response packet block

    mouse_write_command(0xF4); // Enable packet streaming stream mode
    mouse_read_data();         // Read final ACK vector block

    return 1; // Device driver armed and loaded successfully
}

void mouse_bootstrap(void) {
    driver_register(&mouse_hardware_driver);
}
