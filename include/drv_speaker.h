#ifndef AIROS_DRV_SPEAKER_H
#define AIROS_DRV_SPEAKER_H

#include "drv.h"

extern struct drv drv_speaker_device;

void drv_speaker_tone(u32 hz);
void drv_speaker_off(void);
/* Needs the timer driver ready. Returns -1 for frequencies outside 20..20000 Hz. */
int drv_speaker_beep(u32 hz, u32 ms);

#endif
