#ifndef AIROS_DRV_RING_H
#define AIROS_DRV_RING_H

#include "airos_types.h"

/* One producer (an IRQ hook) and one consumer (the main loop). DRV_RING_SIZE must be a power of two. */
#define DRV_RING_SIZE 64u

struct drv_ring {
    volatile u32 head;
    volatile u32 tail;
    u16 buf[DRV_RING_SIZE];
};

static inline void drv_ring_reset(struct drv_ring *r) {
    r->head = 0;
    r->tail = 0;
}

static inline int drv_ring_put(struct drv_ring *r, u16 value) {
    u32 next = (r->head + 1) & (DRV_RING_SIZE - 1);
    if (next == r->tail)
        return -1;
    r->buf[r->head] = value;
    r->head = next;
    return 0;
}

static inline int drv_ring_get(struct drv_ring *r) {
    int value;
    if (r->tail == r->head)
        return -1;
    value = r->buf[r->tail];
    r->tail = (r->tail + 1) & (DRV_RING_SIZE - 1);
    return value;
}

#endif
