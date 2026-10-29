#ifndef AIROS_DRV_PIC_H
#define AIROS_DRV_PIC_H

#include "drv.h"

extern struct drv drv_pic_device;

void drv_pic_mask(int line);
void drv_pic_unmask(int line);
void drv_pic_eoi(int line);

#endif
