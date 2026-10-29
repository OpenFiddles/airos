#ifndef AIROS_DRV_SERIAL_H
#define AIROS_DRV_SERIAL_H

#include "drv.h"

extern struct drv drv_serial_device;

void drv_serial_putc(char c);
void drv_serial_write(const char *s);
int drv_serial_getc(void);

#endif
