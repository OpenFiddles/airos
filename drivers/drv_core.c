#include "../include/drv.h"
#include "../include/drv_pic.h"

static struct drv *table[DRV_MAX];
static int table_n;

static int str_eq(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

int drv_register(struct drv *d) {
    if (!d || !d->name || table_n >= DRV_MAX)
        return -1;
    if (d->irq_line != DRV_NO_IRQ && (d->irq_line < 0 || d->irq_line >= DRV_IRQ_LINES))
        return -1;
    for (int i = 0; i < table_n; i++)
        if (str_eq(table[i]->name, d->name))
            return -1;
    d->ready = 0;
    table[table_n++] = d;
    return 0;
}

int drv_init_all(void) {
    int ready = 0;
    for (int i = 0; i < table_n; i++) {
        struct drv *d = table[i];
        if (!d->ops || !d->ops->init)
            d->ready = 1;
        else
            d->ready = (d->ops->init() == 0);
        ready += d->ready;
    }
    return ready;
}

void drv_shutdown_all(void) {
    for (int i = table_n - 1; i >= 0; i--) {
        struct drv *d = table[i];
        if (d->ready && d->ops && d->ops->shutdown)
            d->ops->shutdown();
        d->ready = 0;
    }
}

void drv_dispatch_irq(int line) {
    if (line < 0 || line >= DRV_IRQ_LINES)
        return;
    for (int i = 0; i < table_n; i++) {
        struct drv *d = table[i];
        if (d->irq_line == line && d->ready && d->ops && d->ops->irq) {
            d->ops->irq();
            break;
        }
    }
    drv_pic_eoi(line);
}

int drv_count(void) {
    return table_n;
}

const struct drv *drv_find(const char *name) {
    for (int i = 0; i < table_n; i++)
        if (str_eq(table[i]->name, name))
            return table[i];
    return 0;
}
