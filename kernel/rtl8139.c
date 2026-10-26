#include "kernel.h"

/* Allocation of Shared Network Context Globals from kernel.h */
u16 net_io = 0;
u8 net_mac[6] = {0, 0, 0, 0, 0, 0};

/* --- RTL8139 Registers & Offsets --- */
#define RTL_REG_MAC0       0x00    // MAC Address Bytes 0-3
#define RTL_REG_MAC4       0x04    // MAC Address Bytes 4-5
#define RTL_REG_RBSTART    0x30    // Receive Buffer Start Address
#define RTL_REG_CR         0x37    // Command Register
#define RTL_REG_CAPR       0x38    // Current Address of Packet Read
#define RTL_REG_IMR        0x3C    // Interrupt Mask Register
#define RTL_REG_ISR        0x3E    // Interrupt Status Register
#define RTL_REG_TCR        0x40    // Transmit Configuration Register
#define RTL_REG_RCR        0x44    // Receive Configuration Register
#define RTL_REG_CONFIG1    0x52    // Configuration Register 1
#define RTL_REG_MSR        0x58    // Media Status Register (Link State)

/* 8KB + 16 bytes (packet header room) + 1.5KB slack alignment buffer */
#define RX_BUF_SIZE 8192
static u8 rx_buffer[RX_BUF_SIZE + 16 + 1500] __attribute__((aligned(4)));

/**
 * Basic helper utility for string copying inside driver structures
 */
static void net_strncpy(char *dest, const char *src, int n) {
    int i = 0;
    while (i < n - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/**
 * Prints the parsed hardware MAC address block to screen
 */
void put_mac(const u8 *mac) {
    for (int i = 0; i < 6; i++) {
        put_hexn(mac[i], 2);
        if (i < 5) putc(':');
    }
}

/**
 * Checks the hardware Media Status Register for valid physical link state connection.
 */
int rtl_link_up(void) {
    if (net_io == 0) return 0;
    
    // Bit 2 (0x04) of Media Status Register (MSR) indicates standard "Link Down" status when set.
    u8 msr = inb(net_io + RTL_REG_MSR);
    return (msr & 0x04) ? 0 : 1; 
}

/**
 * Initializes and maps the RTL8139 Fast Ethernet hardware layout.
 */
int rtl_init(void) {
    /* 
     * NOTE ON THE PCI LAYER: 
     * In a production kernel environment, you would use a PCI scan routine to identify 
     * VendorID 0x10EC / DeviceID 0x8139 and read the Base Address Register (BAR0).
     * For standard desktop emulators (QEMU/Bochs), this defaults to I/O Port 0xC000.
     */
    net_io = 0xC000; 

    // 1. Wake up the chip by turning off power-management sleep states (CONFIG1)
    outb(net_io + RTL_REG_CONFIG1, 0x00);

    // 2. Perform a software reset to clear internal states
    outb(net_io + RTL_REG_CR, 0x10); // Set RST bit
    while ((inb(net_io + RTL_REG_CR) & 0x10) != 0) {
        io_wait(); // Wait until hardware resets the bit back to zero
    }

    // 3. Register our physical memory Receive Buffer array with the chip
    outl(net_io + RTL_REG_RBSTART, (u32)&rx_buffer[0]);

    // 4. Configure the Interrupt Mask Register (IMR)
    // Mask out 0x0005: ROK (Receive OK) and TOK (Transmit OK) interrupt events
    outw(net_io + RTL_REG_IMR, 0x0005); 

    // 5. Configure the Receive Configuration Register (RCR)
    // 0x0000000F: Accept Broadcast, Multicast, Physical Match, and Promiscuous packets
    // 0x00000080: WRAP bit set to allow buffer continuous parsing wrapping safely
    outl(net_io + RTL_REG_RCR, 0x0000008F);

    // 6. Enable the Receiver and Transmitter pipelines inside the Command Register (CR)
    outb(net_io + RTL_REG_CR, 0x0C); // RE (Receiver Enable) | TE (Transmitter Enable)

    // 7. Extract the hardcoded factory MAC Address registers directly off the EEPROM lines
    u32 mac_low = inl(net_io + RTL_REG_MAC0);
    u16 mac_high = inw(net_io + RTL_REG_MAC4);

    net_mac[0] = (u8)(mac_low & 0xFF);
    net_mac[1] = (u8)((mac_low >> 8) & 0xFF);
    net_mac[2] = (u8)((mac_low >> 16) & 0xFF);
    net_mac[3] = (u8)((mac_low >> 24) & 0xFF);
    net_mac[4] = (u8)(mac_high & 0xFF);
    net_mac[5] = (u8)((mac_high >> 8) & 0xFF);

    // Logging output status to local console
    u8 old_attr = cur_attr;
    cur_attr = ATTR_OK;
    puts("[NET] RTL8139 initialized at I/O Port: 0x"); put_hexn(net_io, 4); puts("\n");
    puts("[NET] Hardware MAC Address: "); put_mac(net_mac); puts("\n");
    
    if (rtl_link_up()) {
        puts("[NET] Physical Ethernet link state active.\n");
    } else {
        cur_attr = ATTR_SQR;
        puts("[NET] Link down. Check virtual network attachment flags.\n");
    }
    
    cur_attr = old_attr;
    return 1;
}

/**
 * Standard device shutdown sequence to safely silence DMA channels.
 */
void rtl_shutdown(void) {
    if (net_io != 0) {
        outb(net_io + RTL_REG_CR, 0x00); // Clear Receiver/Transmitter enable bits
    }
}

/* --- Framework Instantiation & Autonomous Binding --- */
struct driver net_driver;

void network_driver_pre_init(void) {
    net_strncpy(net_driver.name, "Realtek RTL8139 NIC", DRIVER_NAME_MAX);
    net_driver.type = DRV_TYPE_NETWORK;
    net_driver.init = rtl_init;
    net_driver.shutdown = rtl_shutdown;
    
    driver_register(&net_driver);
}
