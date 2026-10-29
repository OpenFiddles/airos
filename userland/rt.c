/**
 * rt.c - Runtime Memory Utility Library
 */

/* Type Definitions */
typedef unsigned char u8;
typedef unsigned int u32;

/* Function Declarations (Prototypes) */
void *memset(void *dst, int value, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
void *memmove(void *dst, const void *src, u32 n);
int memcmp(const void *s1, const void *s2, u32 n);

/**
 * Sets a block of memory to a specified byte value.
 * Optimized to use 32-bit (word-wide) writes.
 */
void *memset(void *dst, int value, u32 n) {
    u8 *p = (u8 *)dst;
    
    // Broadcast the 8-bit value across all 4 bytes of a 32-bit word
    u8 b = (u8)value;
    u32 word_value = (b << 24) | (b << 16) | (b << 8) | b;

    // Fill memory in 4-byte chunks
    while (n >= 4) {
        *(u32 *)p = word_value;
        p += 4;
        n -= 4;
    }

    // Handle any remaining trailing bytes
    while (n--) {
        *p++ = b;
    }

    return dst;
}

/**
 * Copies a memory block from source to destination.
 * Optimized to use 32-bit (word-wide) copies.
 * Note: Behavior is undefined if source and destination overlap.
 */
void *memcpy(void *dst, const void *src, u32 n) {
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;

    // Copy memory in 4-byte chunks
    while (n >= 4) {
        *(u32 *)d = *(const u32 *)s;
        d += 4;
        s += 4;
        n -= 4;
    }

    // Handle any remaining trailing bytes
    while (n--) {
        *d++ = *s++;
    }

    return dst;
}

/**
 * Safely copies memory blocks even if source and destination regions overlap.
 */
void *memmove(void *dst, const void *src, u32 n) {
    u8 *d = (u8 *)dst;
    const u8 *s = (const u8 *)src;

    // If memory blocks overlap and dst starts inside src, copy backward
    if (d > s && d < s + n) {
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    } else {
        // Otherwise, forward copy is completely safe
        memcpy(dst, src, n);
    }
    return dst;
}

int memcmp(const void *s1, const void *s2, u32 n) {
    const u8 *p1 = (const u8 *)s1;
    const u8 *p2 = (const u8 *)s2;

    while (n--) {
        if (*p1 != *p2) {
            return (int)(*p1 - *p2);
        }
        p1++;
        p2++;
    }
    return 0;
}
