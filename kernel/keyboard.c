/**
 * keyboard.c - PS/2 Keyboard Driver & Input Ring-Buffer Subsystem
 */

#include "kernel.h"

/* Standard US Keyboard Translation Tables */
static const char kmap[0x3A] = {
    0, 0x1B, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

static const char kmap_shift[0x3A] = {
    0, 0x1B, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

/* Safe Circular Input Buffer Internal State */
static volatile u8 kbuf[256];
static volatile u8 kbuf_head = 0;
static volatile u8 kbuf_tail = 0;
static volatile u8 shift_down = 0;

/**
 * Hardware Keyboard ISR
 * Called directly via IDT Vector 0x21 when a key is pressed or released.
 */
void keyboard_irq(void) {
    u8 sc = inb(0x60);  // Read raw byte from PS/2 Encoder Data Port
    outb(0x20, 0x20);  // Send End-of-Interrupt (EOI) to Master PIC

    // Track state transformations for Shift keys
    if (sc == 0x2A || sc == 0x36) { 
        shift_down = 1; 
        return; 
    }
    if (sc == 0xAA || sc == 0xB6) { 
        shift_down = 0; 
        return; 
    }
    
    // Ignore key release breaks and out-of-bounds scan codes
    if ((sc & 0x80) || sc >= 0x3A) {
        return;
    }

    // Convert raw hardware code to an ASCII character
    char c = shift_down ? kmap_shift[sc] : kmap[sc];
    if (!c) {
        return;
    }

    // Enqueue to ring buffer if room permits
    u8 next = (u8)(kbuf_head + 1);
    if (next == kbuf_tail) {
        return; // Buffer overrun protection (drop character)
    }
    kbuf[kbuf_head] = (u8)c;
    kbuf_head = next;
}

/**
 * Non-blocking character reader for your kernel shell.
 * Returns -1 if the buffer is empty, or the oldest character code.
 */
int kbd_read(void) {
    int c = -1;
    irq_off(); // Critical Section: Disable interrupts to prevent state race condition
    
    if (kbuf_tail != kbuf_head) {
        c = kbuf[kbuf_tail++];
    }
    
    irq_on();  // Restore interrupt processing
    return c;
}
